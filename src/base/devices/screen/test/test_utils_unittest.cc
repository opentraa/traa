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

#include "base/checks.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/rgba_color.h"
#include <gtest/gtest.h>

#include <stdint.h>

namespace traa {
namespace base {

namespace {

void paint_desktop_frame(desktop_frame *frame, desktop_vector pos, rgba_color color) {
  ASSERT_TRUE(frame);
  ASSERT_TRUE(desktop_rect::make_size(frame->size()).contains(pos));
  *reinterpret_cast<uint32_t *>(frame->get_frame_data_at_pos(pos)) = color.to_uint32();
}

// A desktop_frame implementation to store data in heap, but the stide is
// doubled.
class double_size_desktop_frame : public desktop_frame {
public:
  explicit double_size_desktop_frame(desktop_size size);
  ~double_size_desktop_frame() override;
};

double_size_desktop_frame::double_size_desktop_frame(desktop_size size)
    : desktop_frame(size, k_bytes_per_pixel * size.width() * 2,
                    new uint8_t[k_bytes_per_pixel * size.width() * size.height() * 2], nullptr) {}

double_size_desktop_frame::~double_size_desktop_frame() { delete[] data_; }

} // namespace

TEST(test_utils_unittest, basic_data_equals_cases) {
  basic_desktop_frame frame(desktop_size(4, 4));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      paint_desktop_frame(&frame, desktop_vector(i, j), rgba_color(4U * j + i));
    }
  }

  ASSERT_TRUE(desktop_frame_data_equals(frame, frame));
  basic_desktop_frame other(desktop_size(4, 4));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      paint_desktop_frame(&other, desktop_vector(i, j), rgba_color(4U * j + i));
    }
  }
  ASSERT_TRUE(desktop_frame_data_equals(frame, other));
  paint_desktop_frame(&other, desktop_vector(2, 2), rgba_color(0U));
  ASSERT_FALSE(desktop_frame_data_equals(frame, other));
}

TEST(test_utils_unittest, different_size_should_not_equal) {
  basic_desktop_frame frame(desktop_size(4, 4));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      paint_desktop_frame(&frame, desktop_vector(i, j), rgba_color(4U * j + i));
    }
  }

  basic_desktop_frame other(desktop_size(2, 8));
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 8; j++) {
      paint_desktop_frame(&other, desktop_vector(i, j), rgba_color(2U * j + i));
    }
  }

  ASSERT_FALSE(desktop_frame_data_equals(frame, other));
}

TEST(test_utils_unittest, different_stride_should_be_comparable) {
  basic_desktop_frame frame(desktop_size(4, 4));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      paint_desktop_frame(&frame, desktop_vector(i, j), rgba_color(4U * j + i));
    }
  }

  ASSERT_TRUE(desktop_frame_data_equals(frame, frame));
  double_size_desktop_frame other(desktop_size(4, 4));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      paint_desktop_frame(&other, desktop_vector(i, j), rgba_color(4U * j + i));
    }
  }
  ASSERT_TRUE(desktop_frame_data_equals(frame, other));
}

} // namespace base
} // namespace traa
