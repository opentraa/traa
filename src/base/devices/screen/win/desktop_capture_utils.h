/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_CAPTURE_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_CAPTURE_UTILS_H_

#include <comdef.h>

#include <string>

namespace traa {
namespace base {

namespace desktop_capture_utils {
// Generates a human-readable string from a COM error.
std::string com_error_to_string(const _com_error &error);
} // namespace desktop_capture_utils

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_CAPTURE_UTILS_H_
