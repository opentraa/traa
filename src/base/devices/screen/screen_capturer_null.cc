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

namespace traa {
namespace base {

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_screen_capturer(const desktop_capture_options &options) {
  return nullptr;
}

} // namespace base
} // namespace traa
