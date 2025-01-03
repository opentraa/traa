/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/screen_capturer_mac.h"
#include "base/devices/screen/darwin/screen_capturer_sck.h"

#include <memory>

namespace traa {
namespace base {
// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_screen_capturer(const desktop_capture_options &options) {
  if (!options.configuration_monitor()) {
    return nullptr;
  }

  if (options.allow_sck_capturer()) {
    // This will return nullptr on systems that don't support ScreenCaptureKit.
    std::unique_ptr<desktop_capturer> sck_capturer = create_screen_capture_sck(options);
    if (sck_capturer) {
      return sck_capturer;
    }
  }

  auto capturer = std::make_unique<screen_capturer_mac>(
      options.configuration_monitor(), options.detect_updated_region(), options.allow_iosurface());
  if (!capturer->init()) {
    return nullptr;
  }

  return capturer;
}

} // namespace base
} // namespace traa
