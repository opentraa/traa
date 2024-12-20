/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_RGBA_COLOR_H_
#define TRAA_BASE_DEVICES_SCREEN_RGBA_COLOR_H_

#include "base/devices/screen/desktop_frame.h"

#include <stdint.h>

namespace traa {
namespace base {

// A four-byte structure to store a color in BGRA format. This structure also
// provides functions to be created from uint8_t array, say,
// DesktopFrame::data(). It always uses BGRA order for internal storage to match
// DesktopFrame::data().
struct rgba_color final {
  // Creates a color with BGRA channels.
  rgba_color(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha);

  // Creates a color with BGR channels, and set alpha channel to 255 (opaque).
  rgba_color(uint8_t blue, uint8_t green, uint8_t red);

  // Creates a color from four-byte in BGRA order, i.e. DesktopFrame::data().
  explicit rgba_color(const uint8_t *bgra);

  // Creates a color from BGRA channels in a uint format. Consumers should make
  // sure the memory order of the uint32_t is always BGRA from left to right, no
  // matter the system endian. This function creates an equivalent rgba_color
  // instance from the to_uint32() result of another rgba_color instance.
  explicit rgba_color(uint32_t bgra);

  // Returns true if `this` and `right` is the same color.
  bool operator==(const rgba_color &right) const;

  // Returns true if `this` and `right` are different colors.
  bool operator!=(const rgba_color &right) const;

  uint32_t to_uint32() const;

  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t alpha;
};
static_assert(desktop_frame::k_bytes_per_pixel == sizeof(rgba_color),
              "A pixel in DesktopFrame should be safe to be represented by a rgba_color");

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_RGBA_COLOR_H_
