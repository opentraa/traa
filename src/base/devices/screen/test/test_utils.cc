/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/test_utils.h"

#include "base/devices/screen/desktop_geometry.h"

#include <stdint.h>
#include <string.h>

namespace traa {
namespace base {

void clear_desktop_frame(desktop_frame *frame) {
  uint8_t *data = frame->data();
  for (int i = 0; i < frame->size().height(); i++) {
    memset(data, 0, frame->size().width() * desktop_frame::k_bytes_per_pixel);
    data += frame->stride();
  }
}

bool desktop_frame_data_equals(const desktop_frame &left, const desktop_frame &right) {
  if (!left.size().equals(right.size())) {
    return false;
  }

  const uint8_t *left_array = left.data();
  const uint8_t *right_array = right.data();
  for (int i = 0; i < left.size().height(); i++) {
    if (memcmp(left_array, right_array, desktop_frame::k_bytes_per_pixel * left.size().width()) !=
        0) {
      return false;
    }
    left_array += left.stride();
    right_array += right.stride();
  }

  return true;
}

} // namespace base
} // namespace traa
