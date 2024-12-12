/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_FULL_SCREEN_APPLICATION_HANDLER_H_
#define TRAA_BASE_DEVICES_SCREEN_FULL_SCREEN_APPLICATION_HANDLER_H_

#include <memory>

#include "base/devices/screen/desktop_capturer.h"

namespace traa {
namespace base {

// Base class for application specific handler to check criteria for switch to
// full-screen mode and find if possible the full-screen window to share.
// Supposed to be created and owned by platform specific
// full_screen_window_detector.
class full_screen_app_handler {
public:
  virtual ~full_screen_app_handler() {}

  full_screen_app_handler(const full_screen_app_handler &) = delete;
  full_screen_app_handler &operator=(const full_screen_app_handler &) = delete;

  explicit full_screen_app_handler(desktop_capturer::source_id_t sourceId);

  // Returns the full-screen window in place of the original window if all the
  // criteria are met, or 0 if no such window found.
  virtual desktop_capturer::source_id_t
  find_full_screen_window(const desktop_capturer::source_list_t &window_list,
                          int64_t timestamp) const;

  // Returns source id of original window associated with
  // full_screen_app_handler
  desktop_capturer::source_id_t get_source_id() const;

private:
  const desktop_capturer::source_id_t source_id_;
};

} // namespace base
} // namespace traa

#endif // MODULES_DESKTOP_CAPTURE_FULL_SCREEN_APPLICATION_HANDLER_H_
