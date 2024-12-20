/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/cropping_window_capturer.h"

#include "base/devices/screen/desktop_capturer_differ_wrapper.h"
#include "base/devices/screen/full_screen_window_detector.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/selected_window_context.h"
#include "base/log/logger.h"
#include "base/utils/win/version.h"

namespace traa {
namespace base {

inline namespace _impl {

// Used to pass input data for verifying the selected window is on top.
struct top_window_verifer_context : public selected_window_context {
  top_window_verifer_context(HWND selected_window, HWND excluded_window,
                             desktop_rect selected_window_rect,
                             window_capture_helper_win *window_capture_helper)
      : selected_window_context(selected_window, selected_window_rect, window_capture_helper),
        excluded_window(excluded_window) {}

  // Determines whether the selected window is on top (not occluded by any
  // windows except for those it owns or any excluded window).
  bool is_top_window() {
    if (!is_selected_window_valid()) {
      return false;
    }

    // Enumerate all top-level windows above the selected window in Z-order,
    // checking whether any overlaps it. This uses FindWindowEx rather than
    // EnumWindows because the latter excludes certain system windows (e.g. the
    // Start menu & other taskbar menus) that should be detected here to avoid
    // inadvertent capture.
    int num_retries = 0;
    while (true) {
      HWND hwnd = nullptr;
      while ((hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr))) {
        if (hwnd == selected_window()) {
          // Windows are enumerated in top-down Z-order, so we can stop
          // enumerating upon reaching the selected window & report it's on top.
          return true;
        }

        // Ignore the excluded window.
        if (hwnd == excluded_window) {
          continue;
        }

        // Ignore windows that aren't visible on the current desktop.
        if (!get_window_capture_helper()->is_window_visible_on_current_desktop(hwnd)) {
          continue;
        }

        // Ignore Chrome notification windows, especially the notification for
        // the ongoing window sharing. Notes:
        // - This only works with notifications from Chrome, not other Apps.
        // - All notifications from Chrome will be ignored.
        // - This may cause part or whole of notification window being cropped
        // into the capturing of the target window if there is overlapping.
        if (capture_utils::is_window_chrome_notifaction(hwnd)) {
          continue;
        }

        // Ignore windows owned by the selected window since we want to capture
        // them.
        if (is_window_owned_by_selected_window(hwnd)) {
          continue;
        }

        // Check whether this window intersects with the selected window.
        if (is_window_overlapping_selected_window(hwnd)) {
          // If intersection is not empty, the selected window is not on top.
          return false;
        }
      }

      DWORD last_error = ::GetLastError();
      if (last_error == ERROR_SUCCESS) {
        // The enumeration completed successfully without finding the selected
        // window (which may have been closed).
        LOG_WARN("Failed to find selected window (only expected if it was closed)");
        return false;
      } else if (last_error == ERROR_INVALID_WINDOW_HANDLE) {
        // This error may occur if a window is closed around the time it's
        // enumerated; retry the enumeration in this case up to 10 times
        // (this should be a rare race & unlikely to recur).
        if (++num_retries <= 10) {
          LOG_WARN("Enumeration failed due to race with a window closing; retrying - retry #{}",
                   num_retries);
          continue;
        } else {
          LOG_ERROR("Exhausted retry allowance around window enumeration failures due to races "
                    "with windows closing");
        }
      }

      // The enumeration failed with an unexpected error (or more repeats of
      // an infrequently-expected error than anticipated). After logging this &
      // firing an assert when enabled, report that the selected window isn't
      // topmost to avoid inadvertent capture of other windows.
      LOG_ERROR("Failed to enumerate windows: {}", last_error);
      return false;
    }
  }

  const HWND excluded_window;
};

class cropping_window_capturer_win : public cropping_window_capturer {
public:
  explicit cropping_window_capturer_win(const desktop_capture_options &options)
      : cropping_window_capturer(options),
        enumerate_current_process_windows_(options.enumerate_current_process_windows()),
        full_screen_window_detector_(options.get_full_screen_window_detector()) {}

  void capture_frame() override;

private:
  bool should_use_screen_capturer() override;

  desktop_rect get_window_rect_in_virtual_screen() override;

  // Returns either selected by user sourceId or sourceId provided by
  // full_screen_window_detector
  win_id_t get_window_to_capture() const;

  // The region from GetWindowRgn in the desktop coordinate if the region is
  // rectangular, or the rect from GetWindowRect if the region is not set.
  desktop_rect window_region_rect_;

  window_capture_helper_win window_capture_helper_;

  bool enumerate_current_process_windows_;

  std::shared_ptr<full_screen_window_detector> full_screen_window_detector_;

  // Used to make sure that we only log the usage of fullscreen detection once.
  mutable bool fullscreen_usage_logged_ = false;
};

void cropping_window_capturer_win::capture_frame() {
  desktop_capturer *win_capturer = get_window_capturer();
  if (win_capturer) {
    // Feed the actual list of windows into full screen window detector.
    if (full_screen_window_detector_) {
      full_screen_window_detector_->update_window_list_if_needed(
          selected_window(), [this](desktop_capturer::source_list_t *sources) {
            // Get the list of top level windows, including ones with empty
            // title.
            source_list_t result;
            if (!window_capture_helper_.enumerate_capturable_windows(
                    &result, enumerate_current_process_windows_)) {
              LOG_WARN("failed to enumerate capturable windows");
              return false;
            }

            sources->swap(result);
            return true;
          });
    }
    win_capturer->select_source(get_window_to_capture());
  }

  cropping_window_capturer::capture_frame();
}

bool cropping_window_capturer_win::should_use_screen_capturer() {
  if (os_get_version() < version_alias::VERSION_WIN8 &&
      capture_utils::is_dwm_composition_enabled()) {
    return false;
  }

  const HWND selected = reinterpret_cast<HWND>(get_window_to_capture());
  // Check if the window is visible on current desktop.
  if (!window_capture_helper_.is_window_visible_on_current_desktop(selected)) {
    return false;
  }

  // Check if the window is a translucent layered window.
  const LONG window_ex_style = GetWindowLong(selected, GWL_EXSTYLE);
  if (window_ex_style & WS_EX_LAYERED) {
    COLORREF color_ref_key = 0;
    BYTE alpha = 0;
    DWORD flags = 0;

    // ::GetLayeredWindowAttributes fails if the window was setup with
    // ::UpdateLayeredWindow. We have no way to know the opacity of the window in
    // that case. This happens for Stiky Note (crbug/412726).
    if (!::GetLayeredWindowAttributes(selected, &color_ref_key, &alpha, &flags))
      return false;

    // ::UpdateLayeredWindow is the only way to set per-pixel alpha and will cause
    // the previous GetLayeredWindowAttributes to fail. So we only need to check
    // the window wide color key or alpha.
    if ((flags & LWA_COLORKEY) || ((flags & LWA_ALPHA) && (alpha < 255))) {
      return false;
    }
  }

  if (!capture_utils::get_window_rect(selected, &window_region_rect_)) {
    return false;
  }

  desktop_rect content_rect;
  if (!capture_utils::get_window_content_rect(selected, &content_rect)) {
    return false;
  }

  desktop_rect region_rect;
  // Get the window region and check if it is rectangular.
  const int region_type =
      capture_utils::get_window_region_type_with_boundary(selected, &region_rect);

  // Do not use the screen capturer if the region is empty or not rectangular.
  if (region_type == COMPLEXREGION || region_type == NULLREGION) {
    return false;
  }

  if (region_type == SIMPLEREGION) {
    // The `region_rect` returned from GetRgnBox() is always in window
    // coordinate.
    region_rect.translate(window_region_rect_.left(), window_region_rect_.top());
    // MSDN: The window region determines the area *within* the window where the
    // system permits drawing.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd144950(v=vs.85).aspx.
    //
    // `region_rect` should always be inside of `window_region_rect_`. So after
    // the intersection, `window_region_rect_` == `region_rect`. If so, what's
    // the point of the intersecting operations? Why cannot we directly retrieve
    // `window_region_rect_` from capture_utils::get_window_region_type_with_boundary() function?
    // TODO(zijiehe): Figure out the purpose of these intersections.
    window_region_rect_.intersect_with(region_rect);
    content_rect.intersect_with(region_rect);
  }

  // Check if the client area is out of the screen area. When the window is
  // maximized, only its client area is visible in the screen, the border will
  // be hidden. So we are using `content_rect` here.
  if (!capture_utils::get_full_screen_rect().contains(content_rect)) {
    return false;
  }

  // Check if the window is occluded by any other window, excluding the child
  // windows, context menus, and `excluded_window_`.
  // `content_rect` is preferred, see the comments on
  // IsWindowIntersectWithSelectedWindow().
  top_window_verifer_context context(selected, reinterpret_cast<HWND>(excluded_window()),
                                     content_rect, &window_capture_helper_);
  return context.is_top_window();
}

desktop_rect cropping_window_capturer_win::get_window_rect_in_virtual_screen() {
  LOG_EVENT("SDM", "cropping_window_capturer_win::get_window_rect_in_virtual_screen");
  desktop_rect window_rect;
  HWND hwnd = reinterpret_cast<HWND>(get_window_to_capture());
  if (!capture_utils::get_window_cropped_rect(hwnd, /*avoid_cropping_border*/ false, &window_rect,
                                              /*original_rect*/ nullptr)) {
    LOG_WARN("Failed to get window info: {}", GetLastError());
    return window_rect;
  }
  window_rect.intersect_with(window_region_rect_);

  // Convert `window_rect` to be relative to the top-left of the virtual screen.
  desktop_rect screen_rect(capture_utils::get_full_screen_rect());
  window_rect.intersect_with(screen_rect);
  window_rect.translate(-screen_rect.left(), -screen_rect.top());
  return window_rect;
}

win_id_t cropping_window_capturer_win::get_window_to_capture() const {
  const auto selected_source = selected_window();
  const auto full_screen_source =
      full_screen_window_detector_
          ? full_screen_window_detector_->find_full_screen_window(selected_source)
          : 0;
  if (full_screen_source && full_screen_source != selected_source && !fullscreen_usage_logged_) {
    fullscreen_usage_logged_ = true;
    LOG_EVENT("SDM",
              "cropping_window_capturer_win::get_window_to_capture: full screen window detected");
  }
  return full_screen_source ? full_screen_source : selected_source;
}

} // namespace _impl

// static
std::unique_ptr<desktop_capturer>
cropping_window_capturer::create_capturer(const desktop_capture_options &options) {
  std::unique_ptr<desktop_capturer> capturer(new cropping_window_capturer_win(options));
  if (capturer && options.detect_updated_region()) {
    capturer.reset(new desktop_capturer_differ_wrapper(std::move(capturer)));
  }

  return capturer;
}

} // namespace base
} // namespace traa