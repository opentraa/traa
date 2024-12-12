/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/window_capturer_win_gdi.h"

#include "base/arraysize.h"
#include "base/devices/screen/cropped_desktop_frame.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/win/desktop_frame_win.h"
#include "base/devices/screen/win/selected_window_context.h"
#include "base/log/logger.h"
#include "base/utils/time_utils.h"
#include "base/utils/win/version.h"

#include <cmath>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace traa {
namespace base {

// Used to pass input/output data during the EnumWindows call to collect
// owned/pop-up windows that should be captured.
struct owned_window_collector_context : public selected_window_context {
  owned_window_collector_context(HWND selected_window, desktop_rect selected_window_rect,
                                 window_capture_helper_win *capture_helper,
                                 std::vector<HWND> *owned_windows)
      : selected_window_context(selected_window, selected_window_rect, capture_helper),
        owned_windows(owned_windows) {}

  std::vector<HWND> *owned_windows;
};

// Called via EnumWindows for each root window; adds owned/pop-up windows that
// should be captured to a vector it's passed.
BOOL CALLBACK owned_window_collector(HWND hwnd, LPARAM param) {
  owned_window_collector_context *context =
      reinterpret_cast<owned_window_collector_context *>(param);
  if (hwnd == context->selected_window()) {
    // Windows are enumerated in top-down z-order, so we can stop enumerating
    // upon reaching the selected window.
    return FALSE;
  }

  // Skip windows that aren't visible pop-up windows.
  if (!(::GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) ||
      !context->get_window_capture_helper()->is_window_visible_on_current_desktop(hwnd)) {
    return TRUE;
  }

  // Owned windows that intersect the selected window should be captured.
  if (context->is_window_owned_by_selected_window(hwnd) &&
      context->is_window_overlapping_selected_window(hwnd)) {
    // Skip windows that draw shadows around menus. These "SysShadow" windows
    // would otherwise be captured as solid black bars with no transparency
    // gradient (since this capturer doesn't detect / respect variations in the
    // window alpha channel). Any other semi-transparent owned windows will be
    // captured fully-opaque. This seems preferable to excluding them (at least
    // when they have content aside from a solid fill color / visual adornment;
    // e.g. some tooltips have the transparent style set).
    if (::GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
      const WCHAR k_sys_shadow[] = L"SysShadow";
      const size_t k_class_length = arraysize(k_sys_shadow);
      WCHAR class_name[k_class_length];
      const int class_name_length = ::GetClassNameW(hwnd, class_name, k_class_length);
      if (class_name_length == k_class_length - 1 && wcscmp(class_name, k_sys_shadow) == 0) {
        return TRUE;
      }
    }

    context->owned_windows->push_back(hwnd);
  }

  return TRUE;
}

window_capturer_win_gdi::window_capturer_win_gdi(bool enumerate_current_process_windows)
    : enumerate_current_process_windows_(enumerate_current_process_windows) {}
window_capturer_win_gdi::~window_capturer_win_gdi() {}

bool window_capturer_win_gdi::get_source_list(source_list_t *sources) {
  if (!window_capture_helper_.enumerate_capturable_windows(sources,
                                                           enumerate_current_process_windows_))
    return false;

  std::map<HWND, desktop_size> new_map;
  for (const auto &item : *sources) {
    HWND hwnd = reinterpret_cast<HWND>(item.id);
    new_map[hwnd] = window_size_map_[hwnd];
  }
  window_size_map_.swap(new_map);

  return true;
}

bool window_capturer_win_gdi::select_source(source_id_t id) {
  HWND window = reinterpret_cast<HWND>(id);
  if (!capture_utils::is_window_valid_and_visible(window))
    return false;

  window_ = window;
  // When a window is not in the map, window_size_map_[window] will create an
  // item with DesktopSize (0, 0).
  previous_size_ = window_size_map_[window];
  return true;
}

bool window_capturer_win_gdi::focus_on_selected_source() {
  if (!window_)
    return false;

  if (!capture_utils::is_window_valid_and_visible(window_))
    return false;

  return ::BringWindowToTop(window_) && ::SetForegroundWindow(window_);
}

bool window_capturer_win_gdi::is_occluded(const desktop_vector &pos) {
  desktop_vector sys_pos = pos.add(capture_utils::get_full_screen_rect().top_left());
  HWND hwnd = reinterpret_cast<HWND>(window_finder_.get_window_under_point(sys_pos));

  return hwnd != window_ &&
         std::find(owned_windows_.begin(), owned_windows_.end(), hwnd) == owned_windows_.end();
}

void window_capturer_win_gdi::start(capture_callback *callback) {
  LOG_EVENT("SDM", "window_capturer_impl id" + std::to_string(current_capturer_id()));
  callback_ = callback;
}

void window_capturer_win_gdi::capture_frame() {
  int64_t capture_start_time_nanos = time_nanos();

  capture_results results = capture_frame(/*capture_owned_windows*/ true);
  if (!results.frame) {
    // Don't return success if we have no frame.
    results.result = results.result == capture_result::success ? capture_result::error_temporary
                                                               : results.result;
    callback_->on_capture_result(results.result, nullptr);
    return;
  }

  int64_t capture_time_ms = (time_nanos() - capture_start_time_nanos) / k_num_nanosecs_per_millisec;
  results.frame->set_capture_time_ms(capture_time_ms);
  results.frame->set_capturer_id(current_capturer_id());
  callback_->on_capture_result(results.result, std::move(results.frame));
}

window_capturer_win_gdi::capture_results
window_capturer_win_gdi::capture_frame(bool capture_owned_windows) {
  if (!window_) {
    LOG_ERROR("window hasn't been selected: {}", ::GetLastError());
    return {capture_result::error_permanent, nullptr};
  }

  // Stop capturing if the window has been closed.
  if (!::IsWindow(window_)) {
    LOG_ERROR("target window has been closed.");
    return {capture_result::error_permanent, nullptr};
  }

  // Determine the window region excluding any resize border, and including
  // any visible border if capturing an owned window / dialog. (Don't include
  // any visible border for the selected window for consistency with
  // CroppingWindowCapturerWin, which would expose a bit of the background
  // through the partially-transparent border.)
  const bool avoid_cropping_border = !capture_owned_windows;
  desktop_rect cropped_rect;
  desktop_rect original_rect;

  if (!capture_utils::get_window_cropped_rect(window_, avoid_cropping_border, &cropped_rect,
                                              &original_rect)) {
    LOG_WARN("failed to get drawable window area: {}", ::GetLastError());
    return {capture_result::error_temporary, nullptr};
  }

  // Return a 1x1 black frame if the window is minimized or invisible on current
  // desktop, to match behavior on mace. Window can be temporarily invisible
  // during the transition of full screen mode on/off.
  if (original_rect.is_empty() ||
      !window_capture_helper_.is_window_visible_on_current_desktop(window_)) {
    std::unique_ptr<desktop_frame> frame(new basic_desktop_frame(desktop_size(1, 1)));

    previous_size_ = frame->size();
    window_size_map_[window_] = previous_size_;
    return {capture_result::success, std::move(frame)};
  }

  HDC window_dc = ::GetWindowDC(window_);
  if (!window_dc) {
    LOG_WARN("failed to get window DC: {}", ::GetLastError());
    return {capture_result::error_temporary, nullptr};
  }

  desktop_rect unscaled_cropped_rect = cropped_rect;
  double horizontal_scale = 1.0;
  double vertical_scale = 1.0;

  desktop_size window_dc_size;
  if (capture_utils::get_dc_size(window_dc, &window_dc_size)) {
    // The `window_dc_size` is used to detect the scaling of the original
    // window. If the application does not support high-DPI settings, it will
    // be scaled by Windows according to the scaling setting.
    // https://www.google.com/search?q=windows+scaling+settings&ie=UTF-8
    // So the size of the `window_dc`, i.e. the bitmap we can retrieve from
    // PrintWindow() or BitBlt() function, will be smaller than
    // `original_rect` and `cropped_rect`. Part of the captured desktop frame
    // will be black. See
    // bug https://bugs.chromium.org/p/webrtc/issues/detail?id=8112 for
    // details.

    // If `window_dc_size` is smaller than `window_rect`, let's resize both
    // `original_rect` and `cropped_rect` according to the scaling factor.
    // This will adjust the width and height of the two rects.
    horizontal_scale = static_cast<double>(window_dc_size.width()) / original_rect.width();
    vertical_scale = static_cast<double>(window_dc_size.height()) / original_rect.height();
    original_rect.scale(horizontal_scale, vertical_scale);
    cropped_rect.scale(horizontal_scale, vertical_scale);

    // Translate `cropped_rect` to the left so that its position within
    // `original_rect` remains accurate after scaling.
    // See crbug.com/1083527 for more info.
    int translate_left = static_cast<int>(
        std::round((cropped_rect.left() - original_rect.left()) * (horizontal_scale - 1)));
    int translate_top = static_cast<int>(
        std::round((cropped_rect.top() - original_rect.top()) * (vertical_scale - 1)));
    cropped_rect.translate(translate_left, translate_top);
  }

  std::unique_ptr<desktop_frame_win> frame(
      desktop_frame_win::create(original_rect.size(), nullptr, window_dc));
  if (!frame.get()) {
    LOG_WARN("failed to create frame.");
    ::ReleaseDC(window_, window_dc);
    return {capture_result::error_temporary, nullptr};
  }

  HDC mem_dc = ::CreateCompatibleDC(window_dc);
  HGDIOBJ previous_object = ::SelectObject(mem_dc, frame->bitmap());
  BOOL result = FALSE;

  // When desktop composition (Aero) is enabled each window is rendered to a
  // private buffer allowing BitBlt() to get the window content even if the
  // window is occluded. PrintWindow() is slower but lets rendering the window
  // contents to an off-screen device context when Aero is not available.
  // PrintWindow() is not supported by some applications.
  //
  // If Aero is enabled, we prefer BitBlt() because it's faster and avoids
  // window flickering. Otherwise, we prefer PrintWindow() because BitBlt() may
  // render occluding windows on top of the desired window.
  //
  // When composition is enabled the DC returned by GetWindowDC() doesn't always
  // have window frame rendered correctly. Windows renders it only once and then
  // caches the result between captures. We hack it around by calling
  // PrintWindow() whenever window size changes, including the first time of
  // capturing - it somehow affects what we get from BitBlt() on the subsequent
  // captures.
  //
  // For Windows 8.1 and later, we want to always use PrintWindow when the
  // cropping screen capturer falls back to the window capturer. I.e.
  // on Windows 8.1 and later, PrintWindow is only used when the window is
  // occluded. When the window is not occluded, it is much faster to capture
  // the screen and to crop it to the window position and size.
  if (os_get_version() >= version_alias::VERSION_WIN8) {
    // Special flag that makes PrintWindow to work on Windows 8.1 and later.
    // Indeed certain apps (e.g. those using DirectComposition rendering) can't
    // be captured using BitBlt or PrintWindow without this flag. Note that on
    // Windows 8.0 this flag is not supported so the block below will fallback
    // to the other call to PrintWindow. It seems to be very tricky to detect
    // Windows 8.0 vs 8.1 so a try/fallback is more approriate here.
    const UINT flags = PW_RENDERFULLCONTENT;
    result = ::PrintWindow(window_, mem_dc, flags);
  }

  if (!result &&
      (!capture_utils::is_dwm_composition_enabled() || !previous_size_.equals(frame->size()))) {
    result = ::PrintWindow(window_, mem_dc, 0);
  }

  // Aero is enabled or PrintWindow() failed, use BitBlt.
  if (!result) {
    result = ::BitBlt(mem_dc, 0, 0, frame->size().width(), frame->size().height(), window_dc, 0, 0,
                      SRCCOPY);
  }

  ::SelectObject(mem_dc, previous_object);
  ::DeleteDC(mem_dc);
  ::ReleaseDC(window_, window_dc);

  previous_size_ = frame->size();
  window_size_map_[window_] = previous_size_;

  frame->mutable_updated_region()->set_rect(desktop_rect::make_size(frame->size()));
  frame->set_top_left(
      original_rect.top_left().subtract(capture_utils::get_full_screen_rect().top_left()));

  if (!result) {
    LOG_ERROR("both PrintWindow() and BitBlt() failed.");
    return {capture_result::error_temporary, nullptr};
  }

  // Rect for the data is relative to the first pixel of the frame.
  cropped_rect.translate(-original_rect.left(), -original_rect.top());
  std::unique_ptr<desktop_frame> cropped_frame =
      create_cropped_desktop_frame(std::move(frame), cropped_rect);

  if (capture_owned_windows) {
    // If any owned/pop-up windows overlap the selected window, capture them
    // and copy/composite their contents into the frame.
    owned_windows_.clear();
    owned_window_collector_context context(window_, unscaled_cropped_rect, &window_capture_helper_,
                                           &owned_windows_);

    if (context.is_selected_window_valid()) {
      ::EnumWindows(owned_window_collector, reinterpret_cast<LPARAM>(&context));

      if (!owned_windows_.empty()) {
        if (!owned_window_capturer_) {
          owned_window_capturer_ =
              std::make_unique<window_capturer_win_gdi>(enumerate_current_process_windows_);
        }

        // Owned windows are stored in top-down z-order, so this iterates in
        // reverse to capture / draw them in bottom-up z-order
        for (auto it = owned_windows_.rbegin(); it != owned_windows_.rend(); it++) {
          HWND hwnd = *it;
          if (owned_window_capturer_->select_source(reinterpret_cast<source_id_t>(hwnd))) {
            capture_results results = owned_window_capturer_->capture_frame(
                /*capture_owned_windows*/ false);

            if (results.result != desktop_capturer::capture_result::success) {
              // Simply log any error capturing an owned/pop-up window without
              // bubbling it up to the caller (an expected error here is that
              // the owned/pop-up window was closed; any unexpected errors won't
              // fail the outer capture).
              LOG_INFO("capturing owned window failed (previous error/warning pertained to that)");
            } else {
              // Copy / composite the captured frame into the outer frame. This
              // may no-op if they no longer intersect (if the owned window was
              // moved outside the owner bounds since scheduled for capture.)
              cropped_frame->copy_intersecting_pixels_from(*results.frame, horizontal_scale,
                                                           vertical_scale);
            }
          }
        }
      }
    }
  }

  return {capture_result::success, std::move(cropped_frame)};
}

// static
std::unique_ptr<desktop_capturer>
window_capturer_win_gdi::create_raw_window_capturer(const desktop_capture_options &options) {
  return std::unique_ptr<desktop_capturer>(
      new window_capturer_win_gdi(options.enumerate_current_process_windows()));
}

} // namespace base
} // namespace traa
