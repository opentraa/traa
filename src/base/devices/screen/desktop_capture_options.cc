/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/full_screen_application_handler.h"
#include "base/devices/screen/full_screen_window_detector.h"

#if defined(TRAA_OS_MAC) && !defined(TRAA_OS_IOS)
#include "base/devices/screen/darwin/full_screen_mac_application_handler.h"
#elif defined(TRAA_OS_WINDOWS)
#include "base/devices/screen/win/full_screen_win_application_handler.h"
#endif
#if defined(TRAA_ENABLE_WAYLAND)
#include "base/devices/screen/linux/wayland/shared_screencast_stream.h"
#endif

namespace traa {
namespace base {

desktop_capture_options::desktop_capture_options() {}
desktop_capture_options::desktop_capture_options(const desktop_capture_options &options) = default;
desktop_capture_options::desktop_capture_options(desktop_capture_options &&options) = default;
desktop_capture_options::~desktop_capture_options() {}

desktop_capture_options &
desktop_capture_options::operator=(const desktop_capture_options &options) = default;
desktop_capture_options &
desktop_capture_options::operator=(desktop_capture_options &&options) = default;

// static
desktop_capture_options desktop_capture_options::create_default() {
  desktop_capture_options result;
#if defined(TRAA_ENABLE_X11)
  result.set_x_display(shared_x_display::create_default());
#endif
#if defined(TRAA_ENABLE_WAYLAND)
  result.set_screencast_stream(SharedScreenCastStream::CreateDefault());
#endif
#if defined(TRAA_OS_MAC) && !defined(TRAA_OS_IOS)
  result.set_configuration_monitor(std::make_shared<desktop_configuration_monitor>());
  result.set_full_screen_window_detector(
      std::make_shared<full_screen_window_detector>(create_full_screen_app_handler));
#endif

#if defined(TRAA_OS_WINDOWS)
  result.set_full_screen_window_detector(
      std::make_shared<full_screen_window_detector>(create_full_screen_app_handler));
#endif
  return result;
}

} // namespace base
} // namespace traa
