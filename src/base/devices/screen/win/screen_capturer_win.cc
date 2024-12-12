/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <utility>

#include "base/devices/screen/desktop_capturer.h"

#include "base/devices/screen/blank_detector_desktop_capturer_wrapper.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/fallback_desktop_capturer_wrapper.h"
#include "base/devices/screen/rgba_color.h"
#include "base/devices/screen/win/screen_capturer_win_directx.h"
#include "base/devices/screen/win/screen_capturer_win_gdi.h"

namespace traa {
namespace base {

inline namespace _impl {
std::unique_ptr<desktop_capturer>
create_screen_capturer_win_directx(const desktop_capture_options &options) {
  std::unique_ptr<desktop_capturer> capturer(new screen_capturer_win_directx(options));
  capturer.reset(
      new blank_detector_desktop_capturer_wrapper(std::move(capturer), rgba_color(0, 0, 0, 0)));
  return capturer;
}

} // namespace _impl

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_screen_capturer(const desktop_capture_options &options) {
  // Default capturer if no options are enabled is GDI.
  std::unique_ptr<desktop_capturer> capturer(new screen_capturer_win_gdi(options));

  // If DirectX is enabled use it as main capturer with GDI as fallback.
  if (options.allow_directx_capturer()) {
    // `dxgi_duplicator_controller` should be alive in this scope to ensure it
    // won't unload dxgi_duplicator_controller.
    auto dxgi_duplicator_controller = dxgi_duplicator_controller::instance();
    if (screen_capturer_win_directx::is_supported()) {
      capturer.reset(new fallback_desktop_capturer_wrapper(
          create_screen_capturer_win_directx(options), std::move(capturer)));
      return capturer;
    }
  }

  // Use GDI as default capturer without any fallback solution.
  return capturer;
}

} // namespace base
} // namespace traa
