/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_frame_rotation.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/test/test_utils.h"

#include <gtest/gtest.h>

#include <stdint.h>

namespace traa {
namespace base {

namespace {

// A desktop_frame implementation which stores data in an external int array.
class array_desktop_frame : public desktop_frame {
public:
  array_desktop_frame(desktop_size size, uint32_t *data);
  ~array_desktop_frame() override;
};

array_desktop_frame::array_desktop_frame(desktop_size size, uint32_t *data)
    : desktop_frame(size, size.width() * k_bytes_per_pixel, reinterpret_cast<uint8_t *>(data),
                    nullptr) {}

array_desktop_frame::~array_desktop_frame() = default;

} // namespace

TEST(desktop_frame_rotation_unittest, copy_rect_3x4) {
  // A desktop_frame of 4-pixel width by 3-pixel height.
  static uint32_t frame_pixels[] = {
      0, 1, 2,  3,  //
      4, 5, 6,  7,  //
      8, 9, 10, 11, //
  };
  array_desktop_frame frame(desktop_size(4, 3), frame_pixels);

  {
    basic_desktop_frame target(desktop_size(4, 3));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_0,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(frame, target));
  }

  // After Rotating clock-wise 90 degree
  {
    static uint32_t expected_pixels[] = {
        8,  4, 0, //
        9,  5, 1, //
        10, 6, 2, //
        11, 7, 3, //
    };
    array_desktop_frame expected(desktop_size(3, 4), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 4));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_90,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 180 degree
  {
    static uint32_t expected_pixels[] = {
        11, 10, 9, 8, //
        7,  6,  5, 4, //
        3,  2,  1, 0, //
    };
    array_desktop_frame expected(desktop_size(4, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(4, 3));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_180,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 270 degree
  {
    static uint32_t expected_pixels[] = {
        3, 7, 11, //
        2, 6, 10, //
        1, 5, 9,  //
        0, 4, 8,  //
    };
    array_desktop_frame expected(desktop_size(3, 4), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 4));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_270,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }
}

TEST(desktop_frame_rotation_unittest, copy_rect_3x5) {
  // A desktop_frame of 5-pixel width by 3-pixel height.
  static uint32_t frame_pixels[] = {
      0,  1,  2,  3,  4,  //
      5,  6,  7,  8,  9,  //
      10, 11, 12, 13, 14, //
  };
  array_desktop_frame frame(desktop_size(5, 3), frame_pixels);

  {
    basic_desktop_frame target(desktop_size(5, 3));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_0,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, frame));
  }

  // After Rotating clock-wise 90 degree
  {
    static uint32_t expected_pixels[] = {
        10, 5, 0, //
        11, 6, 1, //
        12, 7, 2, //
        13, 8, 3, //
        14, 9, 4, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_90,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 180 degree
  {
    static uint32_t expected_pixels[]{
        14, 13, 12, 11, 10, //
        9,  8,  7,  6,  5,  //
        4,  3,  2,  1,  0,  //
    };
    array_desktop_frame expected(desktop_size(5, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(5, 3));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_180,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 270 degree
  {
    static uint32_t expected_pixels[] = {
        4, 9, 14, //
        3, 8, 13, //
        2, 7, 12, //
        1, 6, 11, //
        0, 5, 10, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_270,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }
}

TEST(desktop_frame_rotation_unittest, partial_copy_rect_3x5) {
  // A desktop_frame of 5-pixel width by 3-pixel height.
  static uint32_t frame_pixels[] = {
      0,  1,  2,  3,  4,  //
      5,  6,  7,  8,  9,  //
      10, 11, 12, 13, 14, //
  };
  array_desktop_frame frame(desktop_size(5, 3), frame_pixels);

  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, 0, 0, //
        0, 6, 7, 8, 0, //
        0, 0, 0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(5, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(5, 3));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 1), rotation::r_0,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  {
    static uint32_t expected_pixels[] = {
        0, 1, 2, 3, 0, //
        0, 6, 7, 8, 0, //
        0, 0, 0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(5, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(5, 3));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 0, 3, 2), rotation::r_0,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 90 degree
  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, //
        0, 6, 0, //
        0, 7, 0, //
        0, 8, 0, //
        0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 1), rotation::r_90,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  {
    static uint32_t expected_pixels[] = {
        0,  0, 0, //
        11, 6, 0, //
        12, 7, 0, //
        13, 8, 0, //
        0,  0, 0, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 2), rotation::r_90,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 180 degree
  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, 0, 0, //
        0, 8, 7, 6, 0, //
        0, 0, 0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(5, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(5, 3));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 1), rotation::r_180,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  {
    static uint32_t expected_pixels[] = {
        0, 13, 12, 11, 0, //
        0, 8,  7,  6,  0, //
        0, 0,  0,  0,  0, //
    };
    array_desktop_frame expected(desktop_size(5, 3), expected_pixels);

    basic_desktop_frame target(desktop_size(5, 3));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 2), rotation::r_180,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  // After Rotating clock-wise 270 degree
  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, //
        0, 8, 0, //
        0, 7, 0, //
        0, 6, 0, //
        0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 1, 3, 1), rotation::r_270,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }

  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, //
        3, 8, 0, //
        2, 7, 0, //
        1, 6, 0, //
        0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(3, 5), expected_pixels);

    basic_desktop_frame target(desktop_size(3, 5));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_xywh(1, 0, 3, 2), rotation::r_270,
                         desktop_vector(), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
  }
}

TEST(desktop_frame_rotation_unittest, with_offset) {
  // A desktop_frame of 4-pixel width by 3-pixel height.
  static uint32_t frame_pixels[] = {
      0, 1, 2,  3,  //
      4, 5, 6,  7,  //
      8, 9, 10, 11, //
  };
  array_desktop_frame frame(desktop_size(4, 3), frame_pixels);

  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, 0,  0,  0, 0, 0, //
        0, 0, 1, 2,  3,  0, 0, 0, //
        0, 4, 5, 6,  7,  0, 0, 0, //
        0, 8, 9, 10, 11, 0, 0, 0, //
        0, 0, 0, 0,  0,  0, 0, 0, //
        0, 0, 0, 0,  0,  0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(8, 6), expected_pixels);

    basic_desktop_frame target(desktop_size(8, 6));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_0,
                         desktop_vector(1, 1), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
    target.mutable_updated_region()->subtract(
        desktop_rect::make_origin_size(desktop_vector(1, 1), frame.size()));
    ASSERT_TRUE(target.updated_region().is_empty());
  }

  {
    static uint32_t expected_pixels[] = {
        0, 0,  0,  0, 0, 0, 0, 0, //
        0, 11, 10, 9, 8, 0, 0, 0, //
        0, 7,  6,  5, 4, 0, 0, 0, //
        0, 3,  2,  1, 0, 0, 0, 0, //
        0, 0,  0,  0, 0, 0, 0, 0, //
        0, 0,  0,  0, 0, 0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(8, 6), expected_pixels);

    basic_desktop_frame target(desktop_size(8, 6));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_180,
                         desktop_vector(1, 1), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
    target.mutable_updated_region()->subtract(
        desktop_rect::make_origin_size(desktop_vector(1, 1), frame.size()));
    ASSERT_TRUE(target.updated_region().is_empty());
  }

  {
    static uint32_t expected_pixels[] = {
        0, 0,  0, 0, 0, 0, //
        0, 8,  4, 0, 0, 0, //
        0, 9,  5, 1, 0, 0, //
        0, 10, 6, 2, 0, 0, //
        0, 11, 7, 3, 0, 0, //
        0, 0,  0, 0, 0, 0, //
        0, 0,  0, 0, 0, 0, //
        0, 0,  0, 0, 0, 0, //
    };
    array_desktop_frame expected(desktop_size(6, 8), expected_pixels);

    basic_desktop_frame target(desktop_size(6, 8));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_90,
                         desktop_vector(1, 1), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
    target.mutable_updated_region()->subtract(desktop_rect::make_xywh(1, 1, 3, 4));
    ASSERT_TRUE(target.updated_region().is_empty());
  }

  {
    static uint32_t expected_pixels[] = {
        0, 0, 0, 0,  0, 0, //
        0, 3, 7, 11, 0, 0, //
        0, 2, 6, 10, 0, 0, //
        0, 1, 5, 9,  0, 0, //
        0, 0, 4, 8,  0, 0, //
        0, 0, 0, 0,  0, 0, //
        0, 0, 0, 0,  0, 0, //
        0, 0, 0, 0,  0, 0, //
    };
    array_desktop_frame expected(desktop_size(6, 8), expected_pixels);

    basic_desktop_frame target(desktop_size(6, 8));
    clear_desktop_frame(&target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_270,
                         desktop_vector(1, 1), &target);
    ASSERT_TRUE(desktop_frame_data_equals(target, expected));
    target.mutable_updated_region()->subtract(desktop_rect::make_xywh(1, 1, 3, 4));
    ASSERT_TRUE(target.updated_region().is_empty());
  }
}

// On a typical machine (Intel(R) Xeon(R) E5-1650 v3 @ 3.50GHz, with O2
// optimization, the following case uses ~1.4s to finish. It means entirely
// rotating one 2048 x 1536 frame, which is a large enough number to cover most
// of desktop computer users, uses around 14ms.
TEST(desktop_frame_rotation_unittest, DISABLED_performance_test) {
  basic_desktop_frame frame(desktop_size(2048, 1536));
  basic_desktop_frame target(desktop_size(1536, 2048));
  basic_desktop_frame target2(desktop_size(2048, 1536));
  for (int i = 0; i < 100; i++) {
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_90,
                         desktop_vector(), &target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_270,
                         desktop_vector(), &target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_0,
                         desktop_vector(), &target2);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_180,
                         desktop_vector(), &target2);
  }
}

// On a typical machine (Intel(R) Xeon(R) E5-1650 v3 @ 3.50GHz, with O2
// optimization, the following case uses ~6.7s to finish. It means entirely
// rotating one 4096 x 3072 frame uses around 67ms.
TEST(desktop_frame_rotation_unittest, DISABLED_performance_test_on_large_screen) {
  basic_desktop_frame frame(desktop_size(4096, 3072));
  basic_desktop_frame target(desktop_size(3072, 4096));
  basic_desktop_frame target2(desktop_size(4096, 3072));
  for (int i = 0; i < 100; i++) {
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_90,
                         desktop_vector(), &target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_270,
                         desktop_vector(), &target);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_0,
                         desktop_vector(), &target2);
    rotate_desktop_frame(frame, desktop_rect::make_size(frame.size()), rotation::r_180,
                         desktop_vector(), &target2);
  }
}

} // namespace base
} // namespace traa
