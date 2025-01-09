/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/screen_capturer_win_directx.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/logger.h"
#include "base/utils/time_utils.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

using Microsoft::WRL::ComPtr;

// static
bool screen_capturer_win_directx::is_supported() {
  // Forwards is_supported() function call to dxgi_duplicator_controller.
  return dxgi_duplicator_controller::instance()->is_supported();
}

// static
bool screen_capturer_win_directx::retrieve_d3d_info(d3d_info *info) {
  // Forwards SupportedFeatureLevels() function call to
  // dxgi_duplicator_controller.
  return dxgi_duplicator_controller::instance()->retrieve_d3d_info(info);
}

// static
bool screen_capturer_win_directx::is_current_session_supported() {
  return dxgi_duplicator_controller::is_current_session_supported();
}

// static
bool screen_capturer_win_directx::get_screen_list_from_device_names(
    const std::vector<std::string> &device_names, desktop_capturer::source_list_t *screens) {
  desktop_capturer::source_list_t gdi_screens;
  std::vector<std::string> gdi_names;
  if (!capture_utils::get_screen_list(&gdi_screens, &gdi_names)) {
    return false;
  }

  screen_id_t max_screen_id = -1;
  for (const desktop_capturer::source_t &screen : gdi_screens) {
    max_screen_id = std::max(max_screen_id, screen.id);
  }

  for (const auto &device_name : device_names) {
    const auto it = std::find(gdi_names.begin(), gdi_names.end(), device_name);
    if (it == gdi_names.end()) {
      // devices_names[i] has not been found in gdi_names, so use max_screen_id.
      max_screen_id++;
      screens->push_back({max_screen_id});
    } else {
      screens->push_back({gdi_screens[it - gdi_names.begin()]});
    }
  }

  return true;
}

// static
int screen_capturer_win_directx::get_index_from_screen_id(
    screen_id_t id, const std::vector<std::string> &device_names) {
  desktop_capturer::source_list_t screens;
  if (!get_screen_list_from_device_names(device_names, &screens)) {
    return -1;
  }

  for (size_t i = 0; i < screens.size(); i++) {
    if (screens[i].id == id) {
      return static_cast<int>(i);
    }
  }

  return -1;
}

screen_capturer_win_directx::screen_capturer_win_directx()
    : controller_(dxgi_duplicator_controller::instance()) {}

screen_capturer_win_directx::screen_capturer_win_directx(const desktop_capture_options &options)
    : screen_capturer_win_directx() {
  options_ = options;
}

screen_capturer_win_directx::~screen_capturer_win_directx() = default;

void screen_capturer_win_directx::start(capture_callback *callback) {
  LOG_INFO("screen_capturer_win_directx::start");

  callback_ = callback;
}

void screen_capturer_win_directx::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> memory_factory) {
  shared_memory_factory_ = std::move(memory_factory);
}

void screen_capturer_win_directx::capture_frame() {
  int64_t capture_start_time_nanos = time_nanos();

  // Note that the [] operator will create the screen_capture_frame_queue if it
  // doesn't exist, so this is safe.
  screen_capture_frame_queue<dxgi_frame> &frames = frame_queue_map_[current_screen_id_];

  frames.move_to_next_frame();

  if (!frames.current_frame()) {
    frames.replace_current_frame(std::make_unique<dxgi_frame>(shared_memory_factory_.get()));
  }

  dxgi_duplicator_controller::duplicate_result result;
  if (current_screen_id_ == k_screen_id_full) {
    result = controller_->duplicate(frames.current_frame());
  } else {
    result = controller_->duplicate_monitor(frames.current_frame(), static_cast<int>(current_screen_id_));
  }

  using duplicate_result = dxgi_duplicator_controller::duplicate_result;
  if (result != duplicate_result::succeeded) {
    LOG_ERROR("dxgi_duplicator_controller failed to capture desktop, error code {}",
              dxgi_duplicator_controller::result_name(result));
  }

  switch (result) {
  case duplicate_result::unsupported_session: {
    LOG_ERROR("Current binary is running on a session not supported "
              "by DirectX screen capturer.");
    callback_->on_capture_result(capture_result::error_permanent, nullptr);
    break;
  }
  case duplicate_result::frame_prepare_failed: {
    LOG_ERROR("Failed to allocate a new DesktopFrame.");
    // This usually means we do not have enough memory or SharedMemoryFactory
    // cannot work correctly.
    callback_->on_capture_result(capture_result::error_permanent, nullptr);
    break;
  }
  case duplicate_result::invalid_monitor_id: {
    LOG_ERROR("Invalid monitor id {}", current_screen_id_);
    callback_->on_capture_result(capture_result::error_permanent, nullptr);
    break;
  }
  case duplicate_result::initialization_failed:
  case duplicate_result::duplication_failed: {
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
    break;
  }
  case duplicate_result::succeeded: {
    std::unique_ptr<desktop_frame> frame = frames.current_frame()->frame()->share();

    int64_t capture_time_ms = (time_nanos() - capture_start_time_nanos) / k_num_nanosecs_per_millisec;

    frame->set_capture_time_ms(capture_time_ms);
    frame->set_capturer_id(desktop_capture_id::k_capture_dxgi);
    // The DXGI Output Duplicator supports embedding the cursor but it is
    // only supported on very few display adapters. This switch allows us
    // to exclude an integrated cursor for all captured frames.
    if (!options_.prefer_cursor_embedded()) {
      frame->set_may_contain_cursor(false);
    }

    // TODO(julien.isorce): http://crbug.com/945468. Set the icc profile on
    // the frame, see WindowCapturerMac::CaptureFrame.

    callback_->on_capture_result(capture_result::success, std::move(frame));
    break;
  }
  }
}

bool screen_capturer_win_directx::get_source_list(source_list_t *sources) {
  std::vector<std::string> device_names;
  if (!controller_->get_device_names(&device_names)) {
    return false;
  }

  return get_screen_list_from_device_names(device_names, sources);
}

bool screen_capturer_win_directx::select_source(source_id_t id) {
  if (id == k_screen_id_full) {
    current_screen_id_ = id;
    return true;
  }

  std::vector<std::string> device_names;
  if (!controller_->get_device_names(&device_names)) {
    return false;
  }

  int index;
  index = get_index_from_screen_id(id, device_names);
  if (index == -1) {
    return false;
  }

  current_screen_id_ = index;
  return true;
}

} // namespace base
} // namespace traa
