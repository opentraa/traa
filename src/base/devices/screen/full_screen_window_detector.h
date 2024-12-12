/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef BASE_DEVICES_SCREEN_FULL_SCREEN_WINDOW_DETECTOR_H_
#define BASE_DEVICES_SCREEN_FULL_SCREEN_WINDOW_DETECTOR_H_

#include <functional>
#include <memory>

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/full_screen_application_handler.h"
#include "base/function_view.h"

namespace traa {
namespace base {

// This is a way to handle switch to full-screen mode for application in some
// specific cases:
// - Chrome on MacOS creates a new window in full-screen mode to
//   show a tab full-screen and minimizes the old window.
// - PowerPoint creates new windows in full-screen mode when user goes to
//   presentation mode (Slide Show Window, Presentation Window).
//
// To continue capturing in these cases, we try to find the new full-screen
// window using criteria provided by application specific full_screen_app_handler.

class full_screen_window_detector {
public:
  using handler_factory_t =
      std::function<std::unique_ptr<full_screen_app_handler>(desktop_capturer::source_id_t)>;

  full_screen_window_detector(handler_factory_t handler_factory);

  full_screen_window_detector(const full_screen_window_detector &) = delete;
  full_screen_window_detector &operator=(const full_screen_window_detector &) = delete;

  // Returns the full-screen window in place of the original window if all the
  // criteria provided by full_screen_app_handler are met, or 0 if no such
  // window found.
  desktop_capturer::source_id_t
  find_full_screen_window(desktop_capturer::source_id_t original_source_id);

  // The caller should call this function periodically, implementation will
  // update internal state no often than twice per second
  void
  update_window_list_if_needed(desktop_capturer::source_id_t original_source_id,
                               function_view<bool(desktop_capturer::source_list_t *)> get_sources);

  static std::shared_ptr<full_screen_window_detector> create_detector();

protected:
  std::unique_ptr<full_screen_app_handler> app_handler_;

private:
  void create_handler_if_needed(desktop_capturer::source_id_t source_id_t);

  handler_factory_t handler_factory_;

  int64_t last_update_time_ms_;
  desktop_capturer::source_id_t previous_source_id_;

  // Save the source id when we fail to create an instance of
  // create_handler_if_needed to avoid redundant attempt to do it again.
  desktop_capturer::source_id_t no_handler_source_id_;

  desktop_capturer::source_list_t window_list_;
};

} // namespace base
} // namespace traa

#endif // MODULES_DESKTOP_CAPTURE_FULL_SCREEN_WINDOW_DETECTOR_H_
