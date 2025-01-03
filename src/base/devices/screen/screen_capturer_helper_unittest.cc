/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/screen_capturer_helper.h"

#include <gtest/gtest.h>

namespace traa {
namespace base {

class screen_capturer_helper_test : public ::testing::Test {
protected:
  screen_capturer_helper capturer_helper_;
};

TEST_F(screen_capturer_helper_test, clear_invalid_region) {
  desktop_region region(desktop_rect::make_xywh(1, 2, 3, 4));
  capturer_helper_.invalidate_region(region);
  capturer_helper_.clear_invalid_region();
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(region.is_empty());
}

TEST_F(screen_capturer_helper_test, invalidate_region) {
  desktop_region region;
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(region.is_empty());

  region.set_rect(desktop_rect::make_xywh(1, 2, 3, 4));
  capturer_helper_.invalidate_region(region);
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(1, 2, 3, 4)).equals(region));

  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(1, 2, 3, 4)));
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(4, 2, 3, 4)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(1, 2, 6, 4)).equals(region));
}

TEST_F(screen_capturer_helper_test, invalidate_screen) {
  desktop_region region;
  capturer_helper_.invalidate_screen(desktop_size(12, 34));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_wh(12, 34)).equals(region));
}

TEST_F(screen_capturer_helper_test, size_most_recent) {
  EXPECT_TRUE(capturer_helper_.size_most_recent().is_empty());
  capturer_helper_.set_size_most_recent(desktop_size(12, 34));
  EXPECT_TRUE(desktop_size(12, 34).equals(capturer_helper_.size_most_recent()));
}

TEST_F(screen_capturer_helper_test, set_log_grid_size) {
  capturer_helper_.set_size_most_recent(desktop_size(10, 10));

  desktop_region region;
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region().equals(region));

  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)).equals(region));

  capturer_helper_.set_log_grid_size(-1);
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)).equals(region));

  capturer_helper_.set_log_grid_size(0);
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)).equals(region));

  capturer_helper_.set_log_grid_size(1);
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);

  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(6, 6, 2, 2)).equals(region));

  capturer_helper_.set_log_grid_size(2);
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(4, 4, 4, 4)).equals(region));

  capturer_helper_.set_log_grid_size(0);
  capturer_helper_.invalidate_region(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)));
  capturer_helper_.take_invalid_region(&region);
  EXPECT_TRUE(desktop_region(desktop_rect::make_xywh(7, 7, 1, 1)).equals(region));
}

void test_expand_region_to_grid(const desktop_region &region, int log_grid_size,
                                const desktop_region &expanded_region_expected) {
  desktop_region expanded_region1;
  screen_capturer_helper::expand_to_grid(region, log_grid_size, &expanded_region1);
  EXPECT_TRUE(expanded_region_expected.equals(expanded_region1));

  desktop_region expanded_region2;
  screen_capturer_helper::expand_to_grid(expanded_region1, log_grid_size, &expanded_region2);
  EXPECT_TRUE(expanded_region1.equals(expanded_region2));
}

void test_expand_rect_to_grid(int l, int t, int r, int b, int log_grid_size, int l_expanded,
                              int t_expanded, int r_expanded, int b_expanded) {
  test_expand_region_to_grid(
      desktop_region(desktop_rect::make_ltrb(l, t, r, b)), log_grid_size,
      desktop_region(desktop_rect::make_ltrb(l_expanded, t_expanded, r_expanded, b_expanded)));
}

TEST_F(screen_capturer_helper_test, expand_to_grid) {
  constexpr int k_log_grid_size = 4;
  constexpr int k_grid_size = 1 << k_log_grid_size;
  for (int i = -2; i <= 2; i++) {
    int x = i * k_grid_size;
    for (int j = -2; j <= 2; j++) {
      int y = j * k_grid_size;
      test_expand_rect_to_grid(x + 0, y + 0, x + 1, y + 1, k_log_grid_size, x + 0, y + 0,
                               x + k_grid_size, y + k_grid_size);
      test_expand_rect_to_grid(x + 0, y + k_grid_size - 1, x + 1, y + k_grid_size, k_log_grid_size,
                               x + 0, y + 0, x + k_grid_size, y + k_grid_size);
      test_expand_rect_to_grid(x + k_grid_size - 1, y + k_grid_size - 1, x + k_grid_size,
                               y + k_grid_size, k_log_grid_size, x + 0, y + 0, x + k_grid_size,
                               y + k_grid_size);
      test_expand_rect_to_grid(x + k_grid_size - 1, y + 0, x + k_grid_size, y + 1, k_log_grid_size,
                               x + 0, y + 0, x + k_grid_size, y + k_grid_size);
      test_expand_rect_to_grid(x - 1, y + 0, x + 1, y + 1, k_log_grid_size, x - k_grid_size, y + 0,
                               x + k_grid_size, y + k_grid_size);
      test_expand_rect_to_grid(x - 1, y - 1, x + 1, y + 0, k_log_grid_size, x - k_grid_size,
                               y - k_grid_size, x + k_grid_size, y);
      test_expand_rect_to_grid(x + 0, y - 1, x + 1, y + 1, k_log_grid_size, x, y - k_grid_size,
                               x + k_grid_size, y + k_grid_size);
      test_expand_rect_to_grid(x - 1, y - 1, x + 0, y + 1, k_log_grid_size, x - k_grid_size,
                               y - k_grid_size, x, y + k_grid_size);

      // Construct a region consisting of 3 pixels and verify that it's expanded
      // properly to 3 squares that are k_grid_size by k_grid_size.
      for (int q = 0; q < 4; ++q) {
        desktop_region region;
        desktop_region expanded_region_expected;

        if (q != 0) {
          region.add_rect(desktop_rect::make_xywh(x - 1, y - 1, 1, 1));
          expanded_region_expected.add_rect(
              desktop_rect::make_xywh(x - k_grid_size, y - k_grid_size, k_grid_size, k_grid_size));
        }
        if (q != 1) {
          region.add_rect(desktop_rect::make_xywh(x, y - 1, 1, 1));
          expanded_region_expected.add_rect(
              desktop_rect::make_xywh(x, y - k_grid_size, k_grid_size, k_grid_size));
        }
        if (q != 2) {
          region.add_rect(desktop_rect::make_xywh(x - 1, y, 1, 1));
          expanded_region_expected.add_rect(
              desktop_rect::make_xywh(x - k_grid_size, y, k_grid_size, k_grid_size));
        }
        if (q != 3) {
          region.add_rect(desktop_rect::make_xywh(x, y, 1, 1));
          expanded_region_expected.add_rect(
              desktop_rect::make_xywh(x, y, k_grid_size, k_grid_size));
        }

        test_expand_region_to_grid(region, k_log_grid_size, expanded_region_expected);
      }
    }
  }
}

} // namespace base
} // namespace traa
