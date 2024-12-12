/* Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/mock_desktop_capturer_callback.h"

namespace traa {
namespace base {

mock_desktop_capturer_callback::mock_desktop_capturer_callback() = default;
mock_desktop_capturer_callback::~mock_desktop_capturer_callback() = default;

void mock_desktop_capturer_callback::on_capture_result(desktop_capturer::capture_result result,
                                                       std::unique_ptr<desktop_frame> frame) {
  on_capture_result_ptr(result, &frame);
}

} // namespace base
} // namespace traa
