/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/cropped_desktop_frame.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include <gtest/gtest.h>

#include <memory>
#include <utility>

namespace traa {
namespace base {

std::unique_ptr<desktop_frame> create_test_frame() {
  return std::make_unique<basic_desktop_frame>(desktop_size(10, 20));
}

TEST(cropped_desktop_frame_test, do_not_create_wrapper_if_size_is_not_changed) {
  std::unique_ptr<desktop_frame> original = create_test_frame();
  // owned by `original` and CroppedDesktopFrame.
  desktop_frame *raw_original = original.get();
  std::unique_ptr<desktop_frame> cropped =
      create_cropped_desktop_frame(std::move(original), desktop_rect::make_wh(10, 20));
  ASSERT_EQ(cropped.get(), raw_original);
}

TEST(cropped_desktop_frame_test, crop_when_partially_out_of_bounds) {
  std::unique_ptr<desktop_frame> cropped =
      create_cropped_desktop_frame(create_test_frame(), desktop_rect::make_wh(11, 10));
  ASSERT_NE(nullptr, cropped);
  ASSERT_EQ(cropped->size().width(), 10);
  ASSERT_EQ(cropped->size().height(), 10);
  ASSERT_EQ(cropped->top_left().x(), 0);
  ASSERT_EQ(cropped->top_left().y(), 0);
}

TEST(cropped_desktop_frame_test, return_null_if_crop_region_is_out_of_bounds) {
  std::unique_ptr<desktop_frame> frame = create_test_frame();
  frame->set_top_left(desktop_vector(100, 200));
  ASSERT_EQ(nullptr, create_cropped_desktop_frame(std::move(frame),
                                                  desktop_rect::make_ltrb(101, 203, 109, 218)));
}

TEST(cropped_desktop_frame_test, crop_a_sub_area) {
  std::unique_ptr<desktop_frame> cropped =
      create_cropped_desktop_frame(create_test_frame(), desktop_rect::make_ltrb(1, 2, 9, 19));
  ASSERT_EQ(cropped->size().width(), 8);
  ASSERT_EQ(cropped->size().height(), 17);
  ASSERT_EQ(cropped->top_left().x(), 1);
  ASSERT_EQ(cropped->top_left().y(), 2);
}

TEST(cropped_desktop_frame_test, set_top_left) {
  std::unique_ptr<desktop_frame> frame = create_test_frame();
  frame->set_top_left(desktop_vector(100, 200));
  frame = create_cropped_desktop_frame(std::move(frame), desktop_rect::make_ltrb(1, 3, 9, 18));
  ASSERT_EQ(frame->size().width(), 8);
  ASSERT_EQ(frame->size().height(), 15);
  ASSERT_EQ(frame->top_left().x(), 101);
  ASSERT_EQ(frame->top_left().y(), 203);
}

TEST(cropped_desktop_frame_test, initialized_with_zeros) {
  std::unique_ptr<desktop_frame> frame = create_test_frame();
  const desktop_vector frame_origin = frame->top_left();
  const desktop_size frame_size = frame->size();
  std::unique_ptr<desktop_frame> cropped = create_cropped_desktop_frame(
      std::move(frame), desktop_rect::make_origin_size(frame_origin, frame_size));
  for (int j = 0; j < cropped->size().height(); ++j) {
    for (int i = 0; i < cropped->stride(); ++i) {
      ASSERT_EQ(cropped->data()[i + j * cropped->stride()], 0);
    }
  }
}

TEST(cropped_desktop_frame_test, icc_profile) {
  const uint8_t fake_icc_profile_data_array[] = {0x1a, 0x00, 0x2b, 0x00, 0x3c, 0x00, 0x4d};
  const std::vector<uint8_t> icc_profile(fake_icc_profile_data_array,
                                         fake_icc_profile_data_array +
                                             sizeof(fake_icc_profile_data_array));

  std::unique_ptr<desktop_frame> frame = create_test_frame();
  EXPECT_EQ(frame->icc_profile().size(), 0UL);

  frame->set_icc_profile(icc_profile);
  EXPECT_EQ(frame->icc_profile().size(), 7UL);
  EXPECT_EQ(frame->icc_profile(), icc_profile);

  frame = create_cropped_desktop_frame(std::move(frame), desktop_rect::make_ltrb(2, 2, 8, 18));
  EXPECT_EQ(frame->icc_profile().size(), 7UL);
  EXPECT_EQ(frame->icc_profile(), icc_profile);

  std::unique_ptr<shared_desktop_frame> shared = shared_desktop_frame::wrap(std::move(frame));
  EXPECT_EQ(shared->icc_profile().size(), 7UL);
  EXPECT_EQ(shared->icc_profile(), icc_profile);

  std::unique_ptr<desktop_frame> shared_other = shared->share();
  EXPECT_EQ(shared_other->icc_profile().size(), 7UL);
  EXPECT_EQ(shared_other->icc_profile(), icc_profile);
}

} // namespace base
} // namespace traa
