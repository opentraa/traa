/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_frame.h"

#include "base/arraysize.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/test/test_utils.h"
#include <gtest/gtest.h>

#include <memory>

namespace traa {
namespace base {

namespace {

std::unique_ptr<desktop_frame> create_test_frame(desktop_rect rect, int pixels_value) {
  desktop_size size = rect.size();
  auto frame = std::make_unique<basic_desktop_frame>(size);
  frame->set_top_left(rect.top_left());
  memset(frame->data(), pixels_value, frame->stride() * size.height());
  return frame;
}

struct test_data {
  const char *description;
  desktop_rect dest_frame_rect;
  desktop_rect src_frame_rect;
  double horizontal_scale;
  double vertical_scale;
  desktop_rect expected_overlap_rect;
};

void run_test(const test_data &test) {
  // Copy a source frame with all bits set into a dest frame with none set.
  auto dest_frame = create_test_frame(test.dest_frame_rect, 0);
  auto src_frame = create_test_frame(test.src_frame_rect, 0xff);

  dest_frame->copy_intersecting_pixels_from(*src_frame, test.horizontal_scale, test.vertical_scale);

  // translate the expected overlap rect to be relative to the dest frame/rect.
  desktop_vector dest_frame_origin = test.dest_frame_rect.top_left();
  desktop_rect relative_expected_overlap_rect = test.expected_overlap_rect;
  relative_expected_overlap_rect.translate(-dest_frame_origin.x(), -dest_frame_origin.y());

  // Confirm bits are now set in the dest frame if & only if they fall in the
  // expected range.
  for (int y = 0; y < dest_frame->size().height(); ++y) {
    SCOPED_TRACE(y);

    for (int x = 0; x < dest_frame->size().width(); ++x) {
      SCOPED_TRACE(x);

      desktop_vector point(x, y);
      uint8_t *data = dest_frame->get_frame_data_at_pos(point);
      uint32_t pixel_value = *reinterpret_cast<uint32_t *>(data);
      bool was_copied = pixel_value == 0xffffffff;
      ASSERT_TRUE(was_copied || pixel_value == 0);

      bool expected_to_be_copied = relative_expected_overlap_rect.contains(point);

      ASSERT_EQ(was_copied, expected_to_be_copied);
    }
  }
}

void run_tests(const test_data *tests, int num_tests) {
  for (int i = 0; i < num_tests; i++) {
    const test_data &test = tests[i];

    SCOPED_TRACE(test.description);

    run_test(test);
  }
}

} // namespace

TEST(desktop_frame_test, new_frame_is_black) {
  auto frame = std::make_unique<basic_desktop_frame>(desktop_size(10, 10));
  EXPECT_TRUE(frame->frame_data_is_black());
}

TEST(desktop_frame_test, empty_frame_is_not_black) {
  auto frame = std::make_unique<basic_desktop_frame>(desktop_size());
  EXPECT_FALSE(frame->frame_data_is_black());
}

TEST(desktop_frame_test, frame_data_switches_between_non_black_and_black) {
  auto frame = create_test_frame(desktop_rect::make_xywh(0, 0, 10, 10), 0xff);
  EXPECT_FALSE(frame->frame_data_is_black());
  frame->set_frame_data_to_black();
  EXPECT_TRUE(frame->frame_data_is_black());
}

TEST(desktop_frame_test, copy_intersecting_pixels_matching_rects) {
  // clang-format off
  const test_data tests[] = {
    {"0 origin",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 2, 2)},

    {"Negative origin",
     desktop_rect::make_xywh(-1, -1, 2, 2),
     desktop_rect::make_xywh(-1, -1, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(-1, -1, 2, 2)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_matching_rects_scaled) {
  // The scale factors shouldn't affect matching rects (they're only applied
  // to any difference between the origins)
  // clang-format off
  const test_data tests[] = {
    {"0 origin 2x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 2, 2),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 0, 2, 2)},

    {"0 origin 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 2, 2),
     0.5, 0.5,
     desktop_rect::make_xywh(0, 0, 2, 2)},

    {"Negative origin 2x",
     desktop_rect::make_xywh(-1, -1, 2, 2),
     desktop_rect::make_xywh(-1, -1, 2, 2),
     2.0, 2.0,
     desktop_rect::make_xywh(-1, -1, 2, 2)},

    {"Negative origin 0.5x",
     desktop_rect::make_xywh(-1, -1, 2, 2),
     desktop_rect::make_xywh(-1, -1, 2, 2),
     0.5, 0.5,
     desktop_rect::make_xywh(-1, -1, 2, 2)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_fully_contained_rects) {
  // clang-format off
  const test_data tests[] = {
    {"0 origin top left",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 1, 1),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 1, 1)},

    {"0 origin bottom right",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(1, 1, 1, 1),
     1.0, 1.0,
     desktop_rect::make_xywh(1, 1, 1, 1)},

    {"Negative origin bottom left",
     desktop_rect::make_xywh(-1, -1, 2, 2),
     desktop_rect::make_xywh(-1, 0, 1, 1),
     1.0, 1.0,
     desktop_rect::make_xywh(-1, 0, 1, 1)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_fully_contained_rects_scaled) {
  // clang-format off
  const test_data tests[] = {
    {"0 origin top left 2x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 1, 1),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 0, 1, 1)},

    {"0 origin top left 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 0, 1, 1),
     0.5, 0.5,
     desktop_rect::make_xywh(0, 0, 1, 1)},

    {"0 origin bottom left 2x",
     desktop_rect::make_xywh(0, 0, 4, 4),
     desktop_rect::make_xywh(1, 1, 2, 2),
     2.0, 2.0,
     desktop_rect::make_xywh(2, 2, 2, 2)},

    {"0 origin bottom middle 2x/1x",
     desktop_rect::make_xywh(0, 0, 4, 3),
     desktop_rect::make_xywh(1, 1, 2, 2),
     2.0, 1.0,
     desktop_rect::make_xywh(2, 1, 2, 2)},

    {"0 origin middle 0.5x",
     desktop_rect::make_xywh(0, 0, 3, 3),
     desktop_rect::make_xywh(2, 2, 1, 1),
     0.5, 0.5,
     desktop_rect::make_xywh(1, 1, 1, 1)},

    {"Negative origin bottom left 2x",
     desktop_rect::make_xywh(-1, -1, 3, 3),
     desktop_rect::make_xywh(-1, 0, 1, 1),
     2.0, 2.0,
     desktop_rect::make_xywh(-1, 1, 1, 1)},

    {"Negative origin near middle 0.5x",
     desktop_rect::make_xywh(-2, -2, 2, 2),
     desktop_rect::make_xywh(0, 0, 1, 1),
     0.5, 0.5,
     desktop_rect::make_xywh(-1, -1, 1, 1)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_partially_contained_rects) {
  // clang-format off
  const test_data tests[] = {
    {"Top left",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-1, -1, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 1, 1)},

    {"Top right",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(1, -1, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(1, 0, 1, 1)},

    {"Bottom right",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(1, 1, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(1, 1, 1, 1)},

    {"Bottom left",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-1, 1, 2, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 1, 1, 1)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_partially_contained_rects_scaled) {
  // clang-format off
  const test_data tests[] = {
    {"Top left 2x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-1, -1, 3, 3),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 0, 1, 1)},

    {"Top right 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(2, -2, 2, 2),
     0.5, 0.5,
     desktop_rect::make_xywh(1, 0, 1, 1)},

    {"Bottom right 2x",
     desktop_rect::make_xywh(0, 0, 3, 3),
     desktop_rect::make_xywh(-1, 1, 3, 3),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 2, 1, 1)},

    {"Bottom left 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-2, 2, 2, 2),
     0.5, 0.5,
     desktop_rect::make_xywh(0, 1, 1, 1)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_uncontained_rects) {
  // clang-format off
  const test_data tests[] = {
    {"Left",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-1, 0, 1, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 0, 0)},

    {"Top",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, -1, 2, 1),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 0, 0)},

    {"Right",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(2, 0, 1, 2),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 0, 0)},


    {"Bottom",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 2, 2, 1),
     1.0, 1.0,
     desktop_rect::make_xywh(0, 0, 0, 0)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

TEST(desktop_frame_test, copy_intersecting_pixels_uncontained_rects_scaled) {
  // clang-format off
  const test_data tests[] = {
    {"Left 2x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(-1, 0, 2, 2),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 0, 0, 0)},

    {"Top 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, -2, 2, 1),
     0.5, 0.5,
     desktop_rect::make_xywh(0, 0, 0, 0)},

    {"Right 2x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(1, 0, 1, 2),
     2.0, 2.0,
     desktop_rect::make_xywh(0, 0, 0, 0)},


    {"Bottom 0.5x",
     desktop_rect::make_xywh(0, 0, 2, 2),
     desktop_rect::make_xywh(0, 4, 2, 1),
     0.5, 0.5,
     desktop_rect::make_xywh(0, 0, 0, 0)}
  };
  // clang-format on

  run_tests(tests, arraysize(tests));
}

} // namespace base
} // namespace traa
