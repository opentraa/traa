/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/blank_detector_desktop_capturer_wrapper.h"

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/test/desktop_frame_generator.h"
#include "base/devices/screen/test/fake_desktop_capturer.h"

#include <gtest/gtest.h>

#include <memory>
#include <utility>

namespace traa {
namespace base {

class blank_detector_desktop_capturer_wrapper_test : public ::testing::Test,
                                                     public desktop_capturer::capture_callback {
public:
  blank_detector_desktop_capturer_wrapper_test();
  ~blank_detector_desktop_capturer_wrapper_test() override;

protected:
  void perf_test(desktop_capturer *capturer);

  const int frame_width_ = 1024;
  const int frame_height_ = 768;
  std::unique_ptr<blank_detector_desktop_capturer_wrapper> wrapper_;
  desktop_capturer *capturer_ = nullptr;
  black_white_desktop_frame_painter painter_;
  int num_frames_captured_ = 0;
  desktop_capturer::capture_result last_result_ = desktop_capturer::capture_result::success;
  std::unique_ptr<desktop_frame> last_frame_;

private:
  // desktop_capturer::capture_callback interface.
  void on_capture_result(desktop_capturer::capture_result result,
                         std::unique_ptr<desktop_frame> frame) override;

  painter_desktop_frame_generator frame_generator_;
};

blank_detector_desktop_capturer_wrapper_test::blank_detector_desktop_capturer_wrapper_test() {
  frame_generator_.size()->set(frame_width_, frame_height_);
  frame_generator_.set_desktop_frame_painter(&painter_);
  std::unique_ptr<desktop_capturer> capturer(new fake_desktop_capturer());
  fake_desktop_capturer *fake_capturer = static_cast<fake_desktop_capturer *>(capturer.get());
  fake_capturer->set_frame_generator(&frame_generator_);
  capturer_ = fake_capturer;
  wrapper_.reset(
      new blank_detector_desktop_capturer_wrapper(std::move(capturer), rgba_color(0, 0, 0, 0)));
  wrapper_->start(this);
}

blank_detector_desktop_capturer_wrapper_test::~blank_detector_desktop_capturer_wrapper_test() =
    default;

void blank_detector_desktop_capturer_wrapper_test::on_capture_result(
    desktop_capturer::capture_result result, std::unique_ptr<desktop_frame> frame) {
  last_result_ = result;
  last_frame_ = std::move(frame);
  num_frames_captured_++;
}

void blank_detector_desktop_capturer_wrapper_test::perf_test(desktop_capturer *capturer) {
  for (int i = 0; i < 10000; i++) {
    capturer->capture_frame();
    ASSERT_EQ(num_frames_captured_, i + 1);
  }
}

TEST_F(blank_detector_desktop_capturer_wrapper_test, should_detect_blank_frame) {
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 1);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::error_temporary);
  ASSERT_FALSE(last_frame_);
}

TEST_F(blank_detector_desktop_capturer_wrapper_test, should_pass_blank_detection) {
  painter_.updated_region()->add_rect(desktop_rect::make_xywh(0, 0, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 1);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);

  painter_.updated_region()->add_rect(
      desktop_rect::make_xywh(frame_width_ - 100, frame_height_ - 100, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 2);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);

  painter_.updated_region()->add_rect(desktop_rect::make_xywh(0, frame_height_ - 100, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 3);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);

  painter_.updated_region()->add_rect(desktop_rect::make_xywh(frame_width_ - 100, 0, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 4);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);

  painter_.updated_region()->add_rect(
      desktop_rect::make_xywh((frame_width_ >> 1) - 50, (frame_height_ >> 1) - 50, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 5);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);
}

TEST_F(blank_detector_desktop_capturer_wrapper_test,
       should_not_check_after_a_non_blank_frame_received) {
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 1);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::error_temporary);
  ASSERT_FALSE(last_frame_);

  painter_.updated_region()->add_rect(desktop_rect::make_xywh(frame_width_ - 100, 0, 100, 100));
  wrapper_->capture_frame();
  ASSERT_EQ(num_frames_captured_, 2);
  ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
  ASSERT_TRUE(last_frame_);

  for (int i = 0; i < 100; i++) {
    wrapper_->capture_frame();
    ASSERT_EQ(num_frames_captured_, i + 3);
    ASSERT_EQ(last_result_, desktop_capturer::capture_result::success);
    ASSERT_TRUE(last_frame_);
  }
}

// There is no perceptible impact by using blank_detector_desktop_capturer_wrapper.
// i.e. less than 0.2ms per frame.
// [ OK ] DISABLED_performance (10210 ms)
// [ OK ] DISABLED_performance_comparison (8791 ms)
TEST_F(blank_detector_desktop_capturer_wrapper_test, DISABLED_performance) {
  perf_test(wrapper_.get());
}

TEST_F(blank_detector_desktop_capturer_wrapper_test, DISABLED_performance_comparison) {
  capturer_->start(this);
  perf_test(capturer_);
}

} // namespace base
} // namespace traa
