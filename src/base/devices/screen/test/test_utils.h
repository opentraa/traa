/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_TEST_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_TEST_UTILS_H_

#include "base/devices/screen/desktop_frame.h"

namespace traa {
namespace base {

// Clears a DesktopFrame `frame` by setting its data() into 0.
void clear_desktop_frame(desktop_frame *frame);

// Compares size() and data() of two DesktopFrames `left` and `right`.
bool desktop_frame_data_equals(const desktop_frame &left, const desktop_frame &right);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_TEST_UTILS_H_
