/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_CROPPED_DESKTOP_FRAME_H_
#define TRAA_BASE_DEVICES_SCREEN_CROPPED_DESKTOP_FRAME_H_

#include <memory>

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

// Creates a desktop_frame to contain only the area of `rect` in the original
// `frame`.
// `frame` should not be nullptr. `rect` is in `frame` coordinate, i.e.
// `frame`->top_left() does not impact the area of `rect`.
// Returns nullptr frame if `rect` is not contained by the bounds of `frame`.
std::unique_ptr<desktop_frame> create_cropped_desktop_frame(std::unique_ptr<desktop_frame> frame,
                                                            const desktop_rect &rect);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_CROPPED_DESKTOP_FRAME_H_
