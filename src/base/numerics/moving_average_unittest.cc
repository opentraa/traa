/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/numerics/moving_average.h"

#include <gtest/gtest.h>

#include <optional>

namespace traa {
namespace base {

TEST(moving_average_test, empty_average) {
  moving_average ma(1);
  EXPECT_EQ(0u, ma.size());
  EXPECT_EQ(std::nullopt, ma.get_average_rounded_down());
}

// Test single value.
TEST(moving_average_test, one_element) {
  moving_average ma(1);
  ma.add_sample(3);
  EXPECT_EQ(1u, ma.size());
  EXPECT_EQ(3, *ma.get_average_rounded_down());
}

TEST(moving_average_test, get_average) {
  moving_average ma(1024);
  ma.add_sample(1);
  ma.add_sample(1);
  ma.add_sample(3);
  ma.add_sample(3);
  EXPECT_EQ(*ma.get_average_rounded_down(), 2);
  EXPECT_EQ(*ma.get_average_rounded_to_closest(), 2);
}

TEST(moving_average_test, get_average_rounded_down_rounds) {
  moving_average ma(1024);
  ma.add_sample(1);
  ma.add_sample(2);
  ma.add_sample(2);
  ma.add_sample(2);
  EXPECT_EQ(*ma.get_average_rounded_down(), 1);
}

TEST(moving_average_test, get_average_rounded_to_closest_rounds) {
  moving_average ma(1024);
  ma.add_sample(1);
  ma.add_sample(2);
  ma.add_sample(2);
  ma.add_sample(2);
  EXPECT_EQ(*ma.get_average_rounded_to_closest(), 2);
}

TEST(moving_average_test, reset) {
  moving_average ma(5);
  ma.add_sample(1);
  EXPECT_EQ(1, *ma.get_average_rounded_down());
  EXPECT_EQ(1, *ma.get_average_rounded_to_closest());

  ma.reset();

  EXPECT_FALSE(ma.get_average_rounded_down());
  ma.add_sample(10);
  EXPECT_EQ(10, *ma.get_average_rounded_down());
  EXPECT_EQ(10, *ma.get_average_rounded_to_closest());
}

TEST(moving_average_test, many_samples) {
  moving_average ma(10);
  for (int i = 1; i < 11; i++) {
    ma.add_sample(i);
  }
  EXPECT_EQ(*ma.get_average_rounded_down(), 5);
  EXPECT_EQ(*ma.get_average_rounded_to_closest(), 6);
  for (int i = 1; i < 2001; i++) {
    ma.add_sample(i);
  }
  EXPECT_EQ(*ma.get_average_rounded_down(), 1995);
  EXPECT_EQ(*ma.get_average_rounded_to_closest(), 1996);
}

} // namespace base
} // namespace traa
