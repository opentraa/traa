/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer.h"

#include "base/devices/screen/blank_detector_desktop_capturer_wrapper.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/fallback_desktop_capturer_wrapper.h"
#include "base/devices/screen/rgba_color.h"
#include "base/devices/screen/win/wgc/wgc_capturer_win.h"
#include "base/devices/screen/win/window_capturer_win_gdi.h"
#include "base/utils/win/version.h"

namespace traa {
namespace base {

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_window_capturer(const desktop_capture_options &options) {
  std::unique_ptr<desktop_capturer> capturer(
      window_capturer_win_gdi::create_raw_window_capturer(options));

  if (options.allow_wgc_capturer_fallback() && os_get_version() >= version_alias::VERSION_WIN11) {
    // blank detector capturer will send an error when it detects a failed
    // GDI rendering, then Fallback capturer will try to capture it again with
    // WGC.
    capturer = std::make_unique<blank_detector_desktop_capturer_wrapper>(
        std::move(capturer), rgba_color(0, 0, 0, 0),
        /*check_per_capture*/ true);

    capturer = std::make_unique<fallback_desktop_capturer_wrapper>(
        std::move(capturer), wgc_capturer_win::create_raw_window_capturer(
                                 options, /*allow_delayed_capturable_check*/ true));
  }

  return capturer;
}

} // namespace base
} // namespace traa
