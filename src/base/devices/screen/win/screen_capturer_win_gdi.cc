/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/screen_capturer_win_gdi.h"

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/mouse_cursor.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/cursor.h"
#include "base/devices/screen/win/desktop_frame_win.h"
#include "base/devices/screen/win/thread_desktop.h"
#include "base/logger.h"
#include "base/utils/time_utils.h"

#include <dwmapi.h>
#include <utility>

namespace traa {
namespace base {

screen_capturer_win_gdi::screen_capturer_win_gdi(const desktop_capture_options &options) {
  disable_effects_ = options.disable_effects();
}

screen_capturer_win_gdi::~screen_capturer_win_gdi() {
  if (desktop_dc_)
    ::ReleaseDC(NULL, desktop_dc_);
  if (memory_dc_)
    ::DeleteDC(memory_dc_);

  if (disable_effects_) {
    // Restore Aero.
    ::DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
  }
}

void screen_capturer_win_gdi::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> memory_factory) {
  shared_memory_factory_ = std::move(memory_factory);
}

void screen_capturer_win_gdi::capture_frame() {
  int64_t capture_start_time_nanos = time_nanos();

  queue_.move_to_next_frame();
  if (queue_.current_frame() && queue_.current_frame()->is_shared()) {
    LOG_WARN("Overwriting frame that is still shared.");
  }

  // Make sure the GDI capture resources are up-to-date.
  prepare_capture_resources();

  if (!capture_image()) {
    LOG_WARN("Failed to capture screen by GDI.");
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
    return;
  }

  // Emit the current frame.
  std::unique_ptr<desktop_frame> frame = queue_.current_frame()->share();
  frame->set_dpi(desktop_vector(::GetDeviceCaps(desktop_dc_, LOGPIXELSX),
                                ::GetDeviceCaps(desktop_dc_, LOGPIXELSY)));
  frame->mutable_updated_region()->set_rect(desktop_rect::make_size(frame->size()));

  int64_t capture_time_ms = (time_nanos() - capture_start_time_nanos) / k_num_nanosecs_per_millisec;
  frame->set_capture_time_ms(capture_time_ms);
  frame->set_capturer_id(desktop_capture_id::k_capture_gdi_screen);
  callback_->on_capture_result(capture_result::success, std::move(frame));
}

bool screen_capturer_win_gdi::get_source_list(source_list_t *sources) {
  return capture_utils::get_screen_list(sources);
}

bool screen_capturer_win_gdi::select_source(source_id_t id) {
  bool valid = capture_utils::is_screen_valid(id, &current_device_key_);
  if (valid)
    current_screen_id_ = id;
  return valid;
}

void screen_capturer_win_gdi::start(capture_callback *callback) {
  LOG_INFO("screen_capturer_impl id" + std::to_string(current_capturer_id()));
  callback_ = callback;

  if (disable_effects_) {
    // Vote to disable Aero composited desktop effects while capturing. Windows
    // will restore Aero automatically if the process exits. This has no effect
    // under Windows 8 or higher.  See crbug.com/124018.
    HRESULT hr = ::DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
    if (FAILED(hr)) {
      LOG_WARN("Failed to disable Aero composition {}", hr);
    }
  }
}

void screen_capturer_win_gdi::prepare_capture_resources() {
  // Switch to the desktop receiving user input if different from the current
  // one.
  std::unique_ptr<thread_desktop> input_desktop(thread_desktop::get_input_desktop());
  if (input_desktop && !desktop_.is_same(*input_desktop)) {
    // Release GDI resources otherwise SetThreadDesktop will fail.
    if (desktop_dc_) {
      ::ReleaseDC(NULL, desktop_dc_);
      desktop_dc_ = nullptr;
    }

    if (memory_dc_) {
      ::DeleteDC(memory_dc_);
      memory_dc_ = nullptr;
    }

    // If SetThreadDesktop() fails, the thread is still assigned a desktop.
    // So we can continue capture screen bits, just from the wrong desktop.
    desktop_.set_thread_desktop(input_desktop.release());

    if (disable_effects_) {
      // Re-assert our vote to disable Aero.
      // See crbug.com/124018 and crbug.com/129906.
      HRESULT hr = ::DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
      if (FAILED(hr)) {
        LOG_WARN("Failed to disable Aero composition {}", hr);
      }
    }
  }

  // If the display configurations have changed then recreate GDI resources.
  if (display_configuration_monitor_.is_changed(k_screen_id_full)) {
    if (desktop_dc_) {
      ::ReleaseDC(NULL, desktop_dc_);
      desktop_dc_ = nullptr;
    }
    if (memory_dc_) {
      ::DeleteDC(memory_dc_);
      memory_dc_ = nullptr;
    }
  }

  if (!desktop_dc_) {
    // Create GDI device contexts to capture from the desktop into memory.
    desktop_dc_ = ::GetDC(nullptr);
    memory_dc_ = ::CreateCompatibleDC(desktop_dc_);

    // Make sure the frame buffers will be reallocated.
    queue_.reset();
  }
}

bool screen_capturer_win_gdi::capture_image() {
  desktop_rect screen_rect =
      capture_utils::get_screen_rect(current_screen_id_, current_device_key_);
  if (screen_rect.is_empty()) {
    LOG_WARN("Failed to get screen rect.");
    return false;
  }

  desktop_size size = screen_rect.size();
  // If the current buffer is from an older generation then allocate a new one.
  // Note that we can't reallocate other buffers at this point, since the caller
  // may still be reading from them.
  if (!queue_.current_frame() || !queue_.current_frame()->size().equals(screen_rect.size())) {
    std::unique_ptr<desktop_frame> buffer =
        desktop_frame_win::create(size, shared_memory_factory_.get(), desktop_dc_);
    if (!buffer) {
      LOG_WARN("Failed to create frame buffer.");
      return false;
    }
    queue_.replace_current_frame(shared_desktop_frame::wrap(std::move(buffer)));
  }
  queue_.current_frame()->set_top_left(
      screen_rect.top_left().subtract(capture_utils::get_full_screen_rect().top_left()));

  // Select the target bitmap into the memory dc and copy the rect from desktop
  // to memory.
  desktop_frame_win *current =
      static_cast<desktop_frame_win *>(queue_.current_frame()->get_underlying_frame());
  HGDIOBJ previous_object = ::SelectObject(memory_dc_, current->bitmap());
  if (!previous_object || previous_object == HGDI_ERROR) {
    LOG_WARN("Failed to select current bitmap into memery dc.");
    return false;
  }

  bool result = (::BitBlt(memory_dc_, 0, 0, screen_rect.width(), screen_rect.height(), desktop_dc_,
                          screen_rect.left(), screen_rect.top(), SRCCOPY | CAPTUREBLT) != FALSE);
  if (!result) {
    LOG_WARN("BitBlt failed");
  }

  // Select back the previously selected object to that the device contect
  // could be destroyed independently of the bitmap if needed.
  ::SelectObject(memory_dc_, previous_object);

  return result;
}

} // namespace base
} // namespace traa
