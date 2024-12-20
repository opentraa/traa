/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/rgba_color.h"

#include "base/arch.h"

namespace traa {
namespace base {

inline namespace {

bool alpha_equals(uint8_t i, uint8_t j) {
  // On Linux and Windows 8 or early version, '0' was returned for alpha channel
  // from capturer APIs, on Windows 10, '255' was returned. So a workaround is
  // to treat 0 as 255.
  return i == j || ((i == 0 || i == 255) && (j == 0 || j == 255));
}

} // namespace

rgba_color::rgba_color(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha) {
  this->blue = blue;
  this->green = green;
  this->red = red;
  this->alpha = alpha;
}

rgba_color::rgba_color(uint8_t blue, uint8_t green, uint8_t red)
    : rgba_color(blue, green, red, 0xff) {}

rgba_color::rgba_color(const uint8_t *bgra) : rgba_color(bgra[0], bgra[1], bgra[2], bgra[3]) {}

rgba_color::rgba_color(uint32_t bgra) : rgba_color(reinterpret_cast<uint8_t *>(&bgra)) {}

bool rgba_color::operator==(const rgba_color &right) const {
  return blue == right.blue && green == right.green && red == right.red &&
         alpha_equals(alpha, right.alpha);
}

bool rgba_color::operator!=(const rgba_color &right) const { return !(*this == right); }

uint32_t rgba_color::to_uint32() const {
#if defined(TRAA_ARCH_LITTLE_ENDIAN)
  return blue | (green << 8) | (red << 16) | (alpha << 24);
#else
  return (blue << 24) | (green << 16) | (red << 8) | alpha;
#endif
}

} // namespace base
} // namespace traa
