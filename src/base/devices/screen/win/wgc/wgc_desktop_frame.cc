/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_desktop_frame.h"

#include <utility>

namespace traa {
namespace base {

wgc_desktop_frame::wgc_desktop_frame(desktop_size size, int stride,
                                     std::vector<uint8_t> &&image_data)
    : desktop_frame(size, stride, image_data.data(), nullptr), image_data_(std::move(image_data)) {}

wgc_desktop_frame::~wgc_desktop_frame() = default;

} // namespace base
} // namespace traa
