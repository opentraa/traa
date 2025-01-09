/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_region.h"

#include <stdlib.h>

#include <algorithm>
#include <cstdint>

#include <gtest/gtest.h>

namespace traa {
namespace base {

namespace {

int random_int(int max) { return (rand() / 256) % max; }

void compare_region(const desktop_region &region, const desktop_rect rects[], int rects_size) {
  desktop_region::iterator it(region);
  for (int i = 0; i < rects_size; ++i) {
    SCOPED_TRACE(i);
    ASSERT_FALSE(it.is_at_end());
    EXPECT_TRUE(it.rect().equals(rects[i]))
        << it.rect().left() << "-" << it.rect().right() << "." << it.rect().top() << "-"
        << it.rect().bottom() << " " << rects[i].left() << "-" << rects[i].right() << "."
        << rects[i].top() << "-" << rects[i].bottom();
    it.advance();
  }
  EXPECT_TRUE(it.is_at_end());
}

} // namespace

// Verify that regions are empty when created.
TEST(desktop_region_test, empty) {
  desktop_region r;
  compare_region(r, NULL, 0);
}

// Verify that empty rectangles are ignored.
TEST(desktop_region_test, add_empty) {
  desktop_region r;
  desktop_rect rect = desktop_rect::make_xywh(1, 2, 0, 0);
  r.add_rect(rect);
  compare_region(r, NULL, 0);
}

// Verify that regions with a single rectangles are handled properly.
TEST(desktop_region_test, single_rect) {
  desktop_region r;
  desktop_rect rect = desktop_rect::make_xywh(1, 2, 3, 4);
  r.add_rect(rect);
  compare_region(r, &rect, 1);
}

// Verify that non-overlapping rectangles are not merged.
TEST(desktop_region_test, non_overlapping_rects) {
  struct Case {
    int count;
    desktop_rect rects[4];
  } cases[] = {
      {1, {desktop_rect::make_xywh(10, 10, 10, 10)}},
      {2, {desktop_rect::make_xywh(10, 10, 10, 10), desktop_rect::make_xywh(30, 10, 10, 15)}},
      {2, {desktop_rect::make_xywh(10, 10, 10, 10), desktop_rect::make_xywh(10, 30, 10, 5)}},
      {3,
       {desktop_rect::make_xywh(10, 10, 10, 9), desktop_rect::make_xywh(30, 10, 15, 10),
        desktop_rect::make_xywh(10, 30, 8, 10)}},
      {4,
       {desktop_rect::make_xywh(0, 0, 30, 10), desktop_rect::make_xywh(40, 0, 10, 30),
        desktop_rect::make_xywh(0, 20, 10, 30), desktop_rect::make_xywh(20, 40, 30, 10)}},
      {4,
       {desktop_rect::make_xywh(0, 0, 10, 100), desktop_rect::make_xywh(20, 10, 30, 10),
        desktop_rect::make_xywh(20, 30, 30, 10), desktop_rect::make_xywh(20, 50, 30, 10)}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(Case)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r;

    for (int j = 0; j < cases[i].count; ++j) {
      r.add_rect(cases[i].rects[j]);
    }
    compare_region(r, cases[i].rects, cases[i].count);

    SCOPED_TRACE("Reverse");

    // Try inserting rects in reverse order.
    r.clear();
    for (int j = cases[i].count - 1; j >= 0; --j) {
      r.add_rect(cases[i].rects[j]);
    }
    compare_region(r, cases[i].rects, cases[i].count);
  }
}

TEST(desktop_region_test, two_rects) {
  struct Case {
    desktop_rect input_rect1;
    desktop_rect input_rect2;
    int expected_count;
    desktop_rect expected_rects[3];
  } cases[] = {
      // Touching rectangles that merge into one.
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(0, 100, 100, 200),
       1,
       {desktop_rect::make_ltrb(0, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(100, 0, 200, 100),
       1,
       {desktop_rect::make_ltrb(100, 0, 200, 200)}},

      // Rectangles touching on the vertical edge.
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(0, 150, 100, 250),
       3,
       {desktop_rect::make_ltrb(100, 100, 200, 150), desktop_rect::make_ltrb(0, 150, 200, 200),
        desktop_rect::make_ltrb(0, 200, 100, 250)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(0, 50, 100, 150),
       3,
       {desktop_rect::make_ltrb(0, 50, 100, 100), desktop_rect::make_ltrb(0, 100, 200, 150),
        desktop_rect::make_ltrb(100, 150, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(0, 120, 100, 180),
       3,
       {desktop_rect::make_ltrb(100, 100, 200, 120), desktop_rect::make_ltrb(0, 120, 200, 180),
        desktop_rect::make_ltrb(100, 180, 200, 200)}},

      // Rectangles touching on the horizontal edge.
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(150, 0, 250, 100),
       2,
       {desktop_rect::make_ltrb(150, 0, 250, 100), desktop_rect::make_ltrb(100, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(50, 0, 150, 100),
       2,
       {desktop_rect::make_ltrb(50, 0, 150, 100), desktop_rect::make_ltrb(100, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(120, 0, 180, 100),
       2,
       {desktop_rect::make_ltrb(120, 0, 180, 100), desktop_rect::make_ltrb(100, 100, 200, 200)}},

      // Overlapping rectangles.
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(50, 50, 150, 150),
       3,
       {desktop_rect::make_ltrb(50, 50, 150, 100), desktop_rect::make_ltrb(50, 100, 200, 150),
        desktop_rect::make_ltrb(100, 150, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(150, 50, 250, 150),
       3,
       {desktop_rect::make_ltrb(150, 50, 250, 100), desktop_rect::make_ltrb(100, 100, 250, 150),
        desktop_rect::make_ltrb(100, 150, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(0, 120, 150, 180),
       3,
       {desktop_rect::make_ltrb(100, 100, 200, 120), desktop_rect::make_ltrb(0, 120, 200, 180),
        desktop_rect::make_ltrb(100, 180, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(120, 0, 180, 150),
       2,
       {desktop_rect::make_ltrb(120, 0, 180, 100), desktop_rect::make_ltrb(100, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 0, 200, 300),
       desktop_rect::make_ltrb(0, 100, 300, 200),
       3,
       {desktop_rect::make_ltrb(100, 0, 200, 100), desktop_rect::make_ltrb(0, 100, 300, 200),
        desktop_rect::make_ltrb(100, 200, 200, 300)}},

      // One rectangle enclosing another.
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(150, 150, 180, 180),
       1,
       {desktop_rect::make_ltrb(100, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(100, 100, 180, 180),
       1,
       {desktop_rect::make_ltrb(100, 100, 200, 200)}},
      {desktop_rect::make_ltrb(100, 100, 200, 200),
       desktop_rect::make_ltrb(150, 150, 200, 200),
       1,
       {desktop_rect::make_ltrb(100, 100, 200, 200)}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(Case)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r;

    r.add_rect(cases[i].input_rect1);
    r.add_rect(cases[i].input_rect2);
    compare_region(r, cases[i].expected_rects, cases[i].expected_count);

    SCOPED_TRACE("Reverse");

    // Run the same test with rectangles inserted in reverse order.
    r.clear();
    r.add_rect(cases[i].input_rect2);
    r.add_rect(cases[i].input_rect1);
    compare_region(r, cases[i].expected_rects, cases[i].expected_count);
  }
}

// Verify that desktop_region::AddRectToRow() works correctly by creating a row
// of not overlapping rectangles and insert an overlapping rectangle into the
// row at different positions. Result is verified by building a map of the
// region in an array and comparing it with the expected values.
TEST(desktop_region_test, same_row) {
  constexpr int k_map_width = 50;
  constexpr int k_last_rect_sizes[] = {3, 27};

  desktop_region base_region;
  bool base_map[k_map_width] = {
      false,
  };

  base_region.add_rect(desktop_rect::make_xywh(5, 0, 5, 1));
  std::fill_n(base_map + 5, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(15, 0, 5, 1));
  std::fill_n(base_map + 15, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(25, 0, 5, 1));
  std::fill_n(base_map + 25, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(35, 0, 5, 1));
  std::fill_n(base_map + 35, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(45, 0, 5, 1));
  std::fill_n(base_map + 45, 5, true);

  for (size_t i = 0; i < sizeof(k_last_rect_sizes) / sizeof(k_last_rect_sizes[0]); i++) {
    int last_rect_size = k_last_rect_sizes[i];
    for (int x = 0; x < k_map_width - last_rect_size; x++) {
      SCOPED_TRACE(x);

      desktop_region r = base_region;
      r.add_rect(desktop_rect::make_xywh(x, 0, last_rect_size, 1));

      bool expected_map[k_map_width];
      std::copy(base_map, base_map + k_map_width, expected_map);
      std::fill_n(expected_map + x, last_rect_size, true);

      bool map[k_map_width] = {
          false,
      };

      int pos = -1;
      for (desktop_region::iterator it(r); !it.is_at_end(); it.advance()) {
        EXPECT_GT(it.rect().left(), pos);
        pos = it.rect().right();
        std::fill_n(map + it.rect().left(), it.rect().width(), true);
      }

      EXPECT_TRUE(std::equal(map, map + k_map_width, expected_map));
    }
  }
}

TEST(desktop_region_test, complex_regions) {
  struct Case {
    int input_count;
    desktop_rect input_rects[4];
    int expected_count;
    desktop_rect expected_rects[6];
  } cases[] = {
      {3,
       {
           desktop_rect::make_ltrb(100, 100, 200, 200),
           desktop_rect::make_ltrb(0, 100, 100, 200),
           desktop_rect::make_ltrb(310, 110, 320, 120),
       },
       2,
       {desktop_rect::make_ltrb(0, 100, 200, 200), desktop_rect::make_ltrb(310, 110, 320, 120)}},
      {3,
       {desktop_rect::make_ltrb(100, 100, 200, 200), desktop_rect::make_ltrb(50, 50, 150, 150),
        desktop_rect::make_ltrb(300, 125, 350, 175)},
       4,
       {desktop_rect::make_ltrb(50, 50, 150, 100), desktop_rect::make_ltrb(50, 100, 200, 150),
        desktop_rect::make_ltrb(300, 125, 350, 175), desktop_rect::make_ltrb(100, 150, 200, 200)}},
      {4,
       {desktop_rect::make_ltrb(0, 0, 30, 30), desktop_rect::make_ltrb(10, 10, 40, 40),
        desktop_rect::make_ltrb(20, 20, 50, 50), desktop_rect::make_ltrb(50, 0, 65, 15)},
       6,
       {desktop_rect::make_ltrb(0, 0, 30, 10), desktop_rect::make_ltrb(50, 0, 65, 15),
        desktop_rect::make_ltrb(0, 10, 40, 20), desktop_rect::make_ltrb(0, 20, 50, 30),
        desktop_rect::make_ltrb(10, 30, 50, 40), desktop_rect::make_ltrb(20, 40, 50, 50)}},
      {3,
       {desktop_rect::make_ltrb(10, 10, 40, 20), desktop_rect::make_ltrb(10, 30, 40, 40),
        desktop_rect::make_ltrb(10, 20, 40, 30)},
       1,
       {desktop_rect::make_ltrb(10, 10, 40, 40)}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(Case)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r;
    r.add_rects(cases[i].input_rects, cases[i].input_count);
    compare_region(r, cases[i].expected_rects, cases[i].expected_count);

    // Try inserting rectangles in reverse order.
    r.clear();
    for (int j = cases[i].input_count - 1; j >= 0; --j) {
      r.add_rect(cases[i].input_rects[j]);
    }
    compare_region(r, cases[i].expected_rects, cases[i].expected_count);
  }
}

TEST(desktop_region_test, equals) {
  struct Region {
    int count;
    desktop_rect rects[4];
    int id;
  } regions[] = {
      // Same region with one of the rectangles 1 pixel wider/taller.
      {2,
       {desktop_rect::make_ltrb(0, 100, 200, 200), desktop_rect::make_ltrb(310, 110, 320, 120)},
       0},
      {2,
       {desktop_rect::make_ltrb(0, 100, 201, 200), desktop_rect::make_ltrb(310, 110, 320, 120)},
       1},
      {2,
       {desktop_rect::make_ltrb(0, 100, 200, 201), desktop_rect::make_ltrb(310, 110, 320, 120)},
       2},

      // Same region with one of the rectangles shifted horizontally and
      // vertically.
      {4,
       {desktop_rect::make_ltrb(0, 0, 30, 30), desktop_rect::make_ltrb(10, 10, 40, 40),
        desktop_rect::make_ltrb(20, 20, 50, 50), desktop_rect::make_ltrb(50, 0, 65, 15)},
       3},
      {4,
       {desktop_rect::make_ltrb(0, 0, 30, 30), desktop_rect::make_ltrb(10, 10, 40, 40),
        desktop_rect::make_ltrb(20, 20, 50, 50), desktop_rect::make_ltrb(50, 1, 65, 16)},
       4},
      {4,
       {desktop_rect::make_ltrb(0, 0, 30, 30), desktop_rect::make_ltrb(10, 10, 40, 40),
        desktop_rect::make_ltrb(20, 20, 50, 50), desktop_rect::make_ltrb(51, 0, 66, 15)},
       5},

      // Same region defined by a different set of rectangles - one of the
      // rectangle is split horizontally into two.
      {3,
       {desktop_rect::make_ltrb(100, 100, 200, 200), desktop_rect::make_ltrb(50, 50, 150, 150),
        desktop_rect::make_ltrb(300, 125, 350, 175)},
       6},
      {4,
       {desktop_rect::make_ltrb(100, 100, 200, 200), desktop_rect::make_ltrb(50, 50, 100, 150),
        desktop_rect::make_ltrb(100, 50, 150, 150), desktop_rect::make_ltrb(300, 125, 350, 175)},
       6},

      // Rectangle region defined by a set of rectangles that merge into one.
      {3,
       {desktop_rect::make_ltrb(10, 10, 40, 20), desktop_rect::make_ltrb(10, 30, 40, 40),
        desktop_rect::make_ltrb(10, 20, 40, 30)},
       7},
      {1, {desktop_rect::make_ltrb(10, 10, 40, 40)}, 7},
  };
  constexpr int k_total_regions = sizeof(regions) / sizeof(Region);

  for (int i = 0; i < k_total_regions; ++i) {
    SCOPED_TRACE(i);

    desktop_region r1(regions[i].rects, regions[i].count);
    for (int j = 0; j < k_total_regions; ++j) {
      SCOPED_TRACE(j);

      desktop_region r2(regions[j].rects, regions[j].count);
      EXPECT_EQ(regions[i].id == regions[j].id, r1.equals(r2));
    }
  }
}

TEST(desktop_region_test, translate) {
  struct case_t {
    int input_count;
    desktop_rect input_rects[4];
    int dx;
    int dy;
    int expected_count;
    desktop_rect expected_rects[5];
  } cases[] = {
      {3,
       {desktop_rect::make_ltrb(0, 0, 30, 30), desktop_rect::make_ltrb(10, 10, 40, 40),
        desktop_rect::make_ltrb(20, 20, 50, 50)},
       3,
       5,
       5,
       {desktop_rect::make_ltrb(3, 5, 33, 15), desktop_rect::make_ltrb(3, 15, 43, 25),
        desktop_rect::make_ltrb(3, 25, 53, 35), desktop_rect::make_ltrb(13, 35, 53, 45),
        desktop_rect::make_ltrb(23, 45, 53, 55)}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(case_t)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r(cases[i].input_rects, cases[i].input_count);
    r.translate(cases[i].dx, cases[i].dy);
    compare_region(r, cases[i].expected_rects, cases[i].expected_count);
  }
}

TEST(desktop_region_test, intersect) {
  struct case_t {
    int input1_count;
    desktop_rect input1_rects[4];
    int input2_count;
    desktop_rect input2_rects[4];
    int expected_count;
    desktop_rect expected_rects[5];
  } cases[] = {
      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(50, 50, 150, 150)},
       1,
       {desktop_rect::make_ltrb(50, 50, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(100, 0, 200, 300)},
       1,
       {desktop_rect::make_ltrb(0, 100, 300, 200)},
       1,
       {desktop_rect::make_ltrb(100, 100, 200, 200)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       2,
       {desktop_rect::make_ltrb(50, 10, 150, 30), desktop_rect::make_ltrb(50, 30, 160, 50)},
       1,
       {desktop_rect::make_ltrb(50, 10, 100, 50)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       2,
       {desktop_rect::make_ltrb(50, 10, 150, 30), desktop_rect::make_ltrb(50, 30, 90, 50)},
       2,
       {desktop_rect::make_ltrb(50, 10, 100, 30), desktop_rect::make_ltrb(50, 30, 90, 50)}},
      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(100, 50, 200, 200)},
       0,
       {}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(case_t)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r1(cases[i].input1_rects, cases[i].input1_count);
    desktop_region r2(cases[i].input2_rects, cases[i].input2_count);

    desktop_region r;
    r.intersect(r1, r2);

    compare_region(r, cases[i].expected_rects, cases[i].expected_count);
  }
}

TEST(desktop_region_test, subtract) {
  struct case_t {
    int input1_count;
    desktop_rect input1_rects[4];
    int input2_count;
    desktop_rect input2_rects[4];
    int expected_count;
    desktop_rect expected_rects[5];
  } cases[] = {
      // subtract one rect from another.
      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(50, 50, 150, 150)},
       2,
       {desktop_rect::make_ltrb(0, 0, 100, 50), desktop_rect::make_ltrb(0, 50, 50, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(-50, -50, 50, 50)},
       2,
       {desktop_rect::make_ltrb(50, 0, 100, 50), desktop_rect::make_ltrb(0, 50, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(-50, 50, 50, 150)},
       2,
       {desktop_rect::make_ltrb(0, 0, 100, 50), desktop_rect::make_ltrb(50, 50, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(50, 50, 150, 70)},
       3,
       {desktop_rect::make_ltrb(0, 0, 100, 50), desktop_rect::make_ltrb(0, 50, 50, 70),
        desktop_rect::make_ltrb(0, 70, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(50, 50, 70, 70)},
       4,
       {desktop_rect::make_ltrb(0, 0, 100, 50), desktop_rect::make_ltrb(0, 50, 50, 70),
        desktop_rect::make_ltrb(70, 50, 100, 70), desktop_rect::make_ltrb(0, 70, 100, 100)}},

      // Empty result.
      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       0,
       {}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(-10, -10, 110, 110)},
       0,
       {}},

      {2,
       {desktop_rect::make_ltrb(0, 0, 100, 100), desktop_rect::make_ltrb(50, 50, 150, 150)},
       2,
       {desktop_rect::make_ltrb(0, 0, 100, 100), desktop_rect::make_ltrb(50, 50, 150, 150)},
       0,
       {}},

      // One rect out of disjoint set.
      {3,
       {desktop_rect::make_ltrb(0, 0, 10, 10), desktop_rect::make_ltrb(20, 20, 30, 30),
        desktop_rect::make_ltrb(40, 0, 50, 10)},
       1,
       {desktop_rect::make_ltrb(20, 20, 30, 30)},
       2,
       {desktop_rect::make_ltrb(0, 0, 10, 10), desktop_rect::make_ltrb(40, 0, 50, 10)}},

      // Row merging.
      {3,
       {desktop_rect::make_ltrb(0, 0, 100, 50), desktop_rect::make_ltrb(0, 50, 150, 70),
        desktop_rect::make_ltrb(0, 70, 100, 100)},
       1,
       {desktop_rect::make_ltrb(100, 50, 150, 70)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)}},

      // No-op subtraction.
      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(100, 0, 200, 100)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(-100, 0, 0, 100)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(0, 100, 0, 200)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)}},

      {1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)},
       1,
       {desktop_rect::make_ltrb(0, -100, 100, 0)},
       1,
       {desktop_rect::make_ltrb(0, 0, 100, 100)}},
  };

  for (size_t i = 0; i < (sizeof(cases) / sizeof(case_t)); ++i) {
    SCOPED_TRACE(i);

    desktop_region r1(cases[i].input1_rects, cases[i].input1_count);
    desktop_region r2(cases[i].input2_rects, cases[i].input2_count);

    r1.subtract(r2);

    compare_region(r1, cases[i].expected_rects, cases[i].expected_count);
  }
}

// Verify that desktop_region::SubtractRows() works correctly by creating a row
// of not overlapping rectangles and subtracting a set of rectangle. Result
// is verified by building a map of the region in an array and comparing it with
// the expected values.
TEST(desktop_region_test, subtract_rect_on_same_row) {
  const int k_map_width = 50;

  struct SpanSet {
    int count;
    struct Range {
      int start;
      int end;
    } spans[3];
  } span_sets[] = {
      {1, {{0, 3}}}, {1, {{0, 5}}}, {1, {{0, 7}}}, {1, {{0, 12}}}, {2, {{0, 3}, {4, 5}, {6, 16}}},
  };

  desktop_region base_region;
  bool base_map[k_map_width] = {
      false,
  };

  base_region.add_rect(desktop_rect::make_xywh(5, 0, 5, 1));
  std::fill_n(base_map + 5, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(15, 0, 5, 1));
  std::fill_n(base_map + 15, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(25, 0, 5, 1));
  std::fill_n(base_map + 25, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(35, 0, 5, 1));
  std::fill_n(base_map + 35, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(45, 0, 5, 1));
  std::fill_n(base_map + 45, 5, true);

  for (size_t i = 0; i < sizeof(span_sets) / sizeof(span_sets[0]); i++) {
    SCOPED_TRACE(i);
    SpanSet &span_set = span_sets[i];
    int span_set_end = span_set.spans[span_set.count - 1].end;
    for (int x = 0; x < k_map_width - span_set_end; ++x) {
      SCOPED_TRACE(x);

      desktop_region r = base_region;

      bool expected_map[k_map_width];
      std::copy(base_map, base_map + k_map_width, expected_map);

      desktop_region region2;
      for (int span = 0; span < span_set.count; span++) {
        std::fill_n(x + expected_map + span_set.spans[span].start,
                    span_set.spans[span].end - span_set.spans[span].start, false);
        region2.add_rect(desktop_rect::make_ltrb(x + span_set.spans[span].start, 0,
                                                 x + span_set.spans[span].end, 1));
      }
      r.subtract(region2);

      bool map[k_map_width] = {
          false,
      };

      int pos = -1;
      for (desktop_region::iterator it(r); !it.is_at_end(); it.advance()) {
        EXPECT_GT(it.rect().left(), pos);
        pos = it.rect().right();
        std::fill_n(map + it.rect().left(), it.rect().width(), true);
      }

      EXPECT_TRUE(std::equal(map, map + k_map_width, expected_map));
    }
  }
}

// Verify that desktop_region::subtract() works correctly by creating a column of
// not overlapping rectangles and subtracting a set of rectangle on the same
// column. Result is verified by building a map of the region in an array and
// comparing it with the expected values.
TEST(desktop_region_test, subtract_rect_on_same_col) {
  constexpr int k_map_height = 50;

  struct span_set_t {
    int count;
    struct Range {
      int start;
      int end;
    } spans[3];
  } span_sets[] = {
      {1, {{0, 3}}}, {1, {{0, 5}}}, {1, {{0, 7}}}, {1, {{0, 12}}}, {2, {{0, 3}, {4, 5}, {6, 16}}},
  };

  desktop_region base_region;
  bool base_map[k_map_height] = {
      false,
  };

  base_region.add_rect(desktop_rect::make_xywh(0, 5, 1, 5));
  std::fill_n(base_map + 5, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(0, 15, 1, 5));
  std::fill_n(base_map + 15, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(0, 25, 1, 5));
  std::fill_n(base_map + 25, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(0, 35, 1, 5));
  std::fill_n(base_map + 35, 5, true);
  base_region.add_rect(desktop_rect::make_xywh(0, 45, 1, 5));
  std::fill_n(base_map + 45, 5, true);

  for (size_t i = 0; i < sizeof(span_sets) / sizeof(span_sets[0]); i++) {
    SCOPED_TRACE(i);
    span_set_t &span_set = span_sets[i];
    int span_set_end = span_set.spans[span_set.count - 1].end;
    for (int y = 0; y < k_map_height - span_set_end; ++y) {
      SCOPED_TRACE(y);

      desktop_region r = base_region;

      bool expected_map[k_map_height];
      std::copy(base_map, base_map + k_map_height, expected_map);

      desktop_region region2;
      for (int span = 0; span < span_set.count; span++) {
        std::fill_n(y + expected_map + span_set.spans[span].start,
                    span_set.spans[span].end - span_set.spans[span].start, false);
        region2.add_rect(desktop_rect::make_ltrb(0, y + span_set.spans[span].start, 1,
                                                 y + span_set.spans[span].end));
      }
      r.subtract(region2);

      bool map[k_map_height] = {
          false,
      };

      int pos = -1;
      for (desktop_region::iterator it(r); !it.is_at_end(); it.advance()) {
        EXPECT_GT(it.rect().top(), pos);
        pos = it.rect().bottom();
        std::fill_n(map + it.rect().top(), it.rect().height(), true);
      }

      for (int j = 0; j < k_map_height; j++) {
        EXPECT_EQ(expected_map[j], map[j]) << "j = " << j;
      }
    }
  }
}

TEST(desktop_region_test, DISABLED_performance) {
  for (int c = 0; c < 1000; ++c) {
    desktop_region r;
    for (int i = 0; i < 10; ++i) {
      r.add_rect(desktop_rect::make_xywh(random_int(1000), random_int(1000), 200, 200));
    }

    for (int i = 0; i < 1000; ++i) {
      r.add_rect(desktop_rect::make_xywh(random_int(1000), random_int(1000), 5 + random_int(10) * 5,
                                         5 + random_int(10) * 5));
    }

    // Iterate over the rectangles.
    for (desktop_region::iterator it(r); !it.is_at_end(); it.advance()) {
    }
  }
}

} // namespace base
} // namespace traa
