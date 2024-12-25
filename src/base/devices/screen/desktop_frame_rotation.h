/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_ROTATION_H_
#define TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_ROTATION_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

// Represents the rotation of a desktop_frame.
enum class rotation {
  r_0,
  r_90,
  r_180,
  r_270,
};

// Rotates input desktop_frame `source`, copies pixel in an unrotated rectangle
// `source_rect` into the target rectangle of another desktop_frame `target`.
// Target rectangle here is the rotated `source_rect` plus `target_offset`.
// `rotation` specifies `source` to `target` rotation. `source_rect` is in
// `source` coordinate. `target_offset` is in `target` coordinate.
// This function triggers check failure if `source` does not cover the
// `source_rect`, or `target` does not cover the rotated `rect`.
void rotate_desktop_frame(const desktop_frame &source, const desktop_rect &source_rect,
                          const rotation &rot, const desktop_vector &target_offset,
                          desktop_frame *target);

// Returns a reverse rotation of `rotation`.
rotation reverse_rotation(rotation rot);

// Returns a rotated desktop_size of `size`.
desktop_size rotate_size(desktop_size size, rotation rot);

// Returns a rotated desktop_rect of `rect`. The `size` represents the size of
// the desktop_frame which `rect` belongs in.
desktop_rect rotate_rect(desktop_rect rect, desktop_size size, rotation rot);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_ROTATION_H_
