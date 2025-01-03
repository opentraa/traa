/*
 *  Copyright (c) 2024 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_SCK_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_SCK_H_

#include <memory>

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"

namespace traa {
namespace base {

// A desktop_capturer implementation that uses ScreenCaptureKit.
std::unique_ptr<desktop_capturer> create_screen_capture_sck(const desktop_capture_options &options);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_SCK_H_
