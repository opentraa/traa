/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_geometry.h"

#include <gtest/gtest.h>

namespace traa {
namespace base {

TEST(desktop_rect_test, union_between_two_non_empty_rects) {
  desktop_rect rect = desktop_rect::make_ltrb(1, 1, 2, 2);
  rect.union_with(desktop_rect::make_ltrb(-2, -2, -1, -1));
  ASSERT_TRUE(rect.equals(desktop_rect::make_ltrb(-2, -2, 2, 2)));
}

TEST(desktop_rect_test, union_with_empty_rect) {
  desktop_rect rect = desktop_rect::make_wh(1, 1);
  rect.union_with(desktop_rect());
  ASSERT_TRUE(rect.equals(desktop_rect::make_wh(1, 1)));

  rect = desktop_rect::make_xywh(1, 1, 2, 2);
  rect.union_with(desktop_rect());
  ASSERT_TRUE(rect.equals(desktop_rect::make_xywh(1, 1, 2, 2)));

  rect = desktop_rect::make_xywh(1, 1, 2, 2);
  rect.union_with(desktop_rect::make_xywh(3, 3, 0, 0));
  ASSERT_TRUE(rect.equals(desktop_rect::make_xywh(1, 1, 2, 2)));
}

TEST(desktop_rect_test, empty_rect_union_with_non_empty_one) {
  desktop_rect rect;
  rect.union_with(desktop_rect::make_wh(1, 1));
  ASSERT_TRUE(rect.equals(desktop_rect::make_wh(1, 1)));

  rect = desktop_rect();
  rect.union_with(desktop_rect::make_xywh(1, 1, 2, 2));
  ASSERT_TRUE(rect.equals(desktop_rect::make_xywh(1, 1, 2, 2)));

  rect = desktop_rect::make_xywh(3, 3, 0, 0);
  rect.union_with(desktop_rect::make_xywh(1, 1, 2, 2));
  ASSERT_TRUE(rect.equals(desktop_rect::make_xywh(1, 1, 2, 2)));
}

TEST(desktop_rect_test, empty_rect_union_with_empty_one) {
  desktop_rect rect;
  rect.union_with(desktop_rect());
  ASSERT_TRUE(rect.is_empty());

  rect = desktop_rect::make_xywh(1, 1, 0, 0);
  rect.union_with(desktop_rect());
  ASSERT_TRUE(rect.is_empty());

  rect = desktop_rect();
  rect.union_with(desktop_rect::make_xywh(1, 1, 0, 0));
  ASSERT_TRUE(rect.is_empty());

  rect = desktop_rect::make_xywh(1, 1, 0, 0);
  rect.union_with(desktop_rect::make_xywh(-1, -1, 0, 0));
  ASSERT_TRUE(rect.is_empty());
}

TEST(desktop_rect_test, scale) {
  desktop_rect rect = desktop_rect::make_xywh(100, 100, 100, 100);
  rect.scale(1.1, 1.1);
  ASSERT_EQ(rect.top(), 100);
  ASSERT_EQ(rect.left(), 100);
  ASSERT_EQ(rect.width(), 110);
  ASSERT_EQ(rect.height(), 110);

  rect = desktop_rect::make_xywh(100, 100, 100, 100);
  rect.scale(0.01, 0.01);
  ASSERT_EQ(rect.top(), 100);
  ASSERT_EQ(rect.left(), 100);
  ASSERT_EQ(rect.width(), 1);
  ASSERT_EQ(rect.height(), 1);

  rect = desktop_rect::make_xywh(100, 100, 100, 100);
  rect.scale(1.1, 0.9);
  ASSERT_EQ(rect.top(), 100);
  ASSERT_EQ(rect.left(), 100);
  ASSERT_EQ(rect.width(), 110);
  ASSERT_EQ(rect.height(), 90);

  rect = desktop_rect::make_xywh(0, 0, 100, 100);
  rect.scale(1.1, 1.1);
  ASSERT_EQ(rect.top(), 0);
  ASSERT_EQ(rect.left(), 0);
  ASSERT_EQ(rect.width(), 110);
  ASSERT_EQ(rect.height(), 110);

  rect = desktop_rect::make_xywh(0, 100, 100, 100);
  rect.scale(1.1, 1.1);
  ASSERT_EQ(rect.top(), 100);
  ASSERT_EQ(rect.left(), 0);
  ASSERT_EQ(rect.width(), 110);
  ASSERT_EQ(rect.height(), 110);
}

} // namespace base
} // namespace traa
