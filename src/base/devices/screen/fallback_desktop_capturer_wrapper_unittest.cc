/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/fallback_desktop_capturer_wrapper.h"

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/test/desktop_frame_generator.h"
#include "base/devices/screen/test/fake_desktop_capturer.h"

#include <gtest/gtest.h>

#include <memory>
#include <stddef.h>
#include <utility>
#include <vector>

namespace traa {
namespace base {

inline namespace _impl {

std::unique_ptr<desktop_capturer>
create_desktop_capturer(painter_desktop_frame_generator *frame_generator) {
  std::unique_ptr<fake_desktop_capturer> capturer(new fake_desktop_capturer());
  capturer->set_frame_generator(frame_generator);
  return capturer;
}

class fake_shared_memory : public shared_memory {
public:
  explicit fake_shared_memory(size_t size);
  ~fake_shared_memory() override;

private:
  static int next_id_;
};

// static
int fake_shared_memory::next_id_ = 0;

fake_shared_memory::fake_shared_memory(size_t size)
    : shared_memory(new char[size], size, 0, next_id_++) {}

fake_shared_memory::~fake_shared_memory() { delete[] static_cast<char *>(data_); }

class fake_shared_memory_factory : public shared_memory_factory {
public:
  fake_shared_memory_factory() = default;
  ~fake_shared_memory_factory() override = default;

  std::unique_ptr<shared_memory> create_shared_memory(size_t size) override;
};

std::unique_ptr<shared_memory> fake_shared_memory_factory::create_shared_memory(size_t size) {
  return std::unique_ptr<shared_memory>(new fake_shared_memory(size));
}

} // namespace _impl

class fallback_desktop_capturer_wrapper_test : public ::testing::Test,
                                               public desktop_capturer::capture_callback {
public:
  fallback_desktop_capturer_wrapper_test();
  ~fallback_desktop_capturer_wrapper_test() override = default;

protected:
  std::vector<std::pair<desktop_capturer::capture_result, bool>> results_;
  fake_desktop_capturer *main_capturer_ = nullptr;
  fake_desktop_capturer *secondary_capturer_ = nullptr;
  std::unique_ptr<fallback_desktop_capturer_wrapper> wrapper_;

private:
  // desktop_capturer::capture_callback interface
  void on_capture_result(desktop_capturer::capture_result result,
                         std::unique_ptr<desktop_frame> frame) override;
  painter_desktop_frame_generator frame_generator_;
};

fallback_desktop_capturer_wrapper_test::fallback_desktop_capturer_wrapper_test() {
  frame_generator_.size()->set(1024, 768);
  std::unique_ptr<desktop_capturer> main_capturer = create_desktop_capturer(&frame_generator_);
  std::unique_ptr<desktop_capturer> secondary_capturer = create_desktop_capturer(&frame_generator_);
  main_capturer_ = static_cast<fake_desktop_capturer *>(main_capturer.get());
  secondary_capturer_ = static_cast<fake_desktop_capturer *>(secondary_capturer.get());
  wrapper_.reset(new fallback_desktop_capturer_wrapper(std::move(main_capturer),
                                                       std::move(secondary_capturer)));
  wrapper_->start(this);
}

void fallback_desktop_capturer_wrapper_test::on_capture_result(
    desktop_capturer::capture_result result, std::unique_ptr<desktop_frame> frame) {
  results_.emplace_back(result, !!frame);
}

TEST_F(fallback_desktop_capturer_wrapper_test, main_never_failed) {
  wrapper_->capture_frame();
  ASSERT_EQ(main_capturer_->num_capture_attempts(), 1);
  ASSERT_EQ(main_capturer_->num_frames_captured(), 1);
  ASSERT_EQ(secondary_capturer_->num_capture_attempts(), 0);
  ASSERT_EQ(secondary_capturer_->num_frames_captured(), 0);
  ASSERT_EQ(results_.size(), 1U);
  ASSERT_EQ(results_[0], std::make_pair(desktop_capturer::capture_result::success, true));
}

TEST_F(fallback_desktop_capturer_wrapper_test, main_failed_temporarily) {
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::error_temporary);
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::success);
  wrapper_->capture_frame();

  ASSERT_EQ(main_capturer_->num_capture_attempts(), 3);
  ASSERT_EQ(main_capturer_->num_frames_captured(), 2);
  ASSERT_EQ(secondary_capturer_->num_capture_attempts(), 1);
  ASSERT_EQ(secondary_capturer_->num_frames_captured(), 1);
  ASSERT_EQ(results_.size(), 3U);
  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(results_[i], std::make_pair(desktop_capturer::capture_result::success, true));
  }
}

TEST_F(fallback_desktop_capturer_wrapper_test, main_failed_permanently) {
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::error_permanent);
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::success);
  wrapper_->capture_frame();

  ASSERT_EQ(main_capturer_->num_capture_attempts(), 2);
  ASSERT_EQ(main_capturer_->num_frames_captured(), 1);
  ASSERT_EQ(secondary_capturer_->num_capture_attempts(), 2);
  ASSERT_EQ(secondary_capturer_->num_frames_captured(), 2);
  ASSERT_EQ(results_.size(), 3U);
  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(results_[i], std::make_pair(desktop_capturer::capture_result::success, true));
  }
}

TEST_F(fallback_desktop_capturer_wrapper_test, both_failed) {
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::error_permanent);
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::success);
  wrapper_->capture_frame();
  secondary_capturer_->set_result(desktop_capturer::capture_result::error_temporary);
  wrapper_->capture_frame();
  secondary_capturer_->set_result(desktop_capturer::capture_result::error_permanent);
  wrapper_->capture_frame();
  wrapper_->capture_frame();

  ASSERT_EQ(main_capturer_->num_capture_attempts(), 2);
  ASSERT_EQ(main_capturer_->num_frames_captured(), 1);
  ASSERT_EQ(secondary_capturer_->num_capture_attempts(), 5);
  ASSERT_EQ(secondary_capturer_->num_frames_captured(), 2);
  ASSERT_EQ(results_.size(), 6U);
  for (int i = 0; i < 3; i++) {
    ASSERT_EQ(results_[i], std::make_pair(desktop_capturer::capture_result::success, true));
  }
  ASSERT_EQ(results_[3], std::make_pair(desktop_capturer::capture_result::error_temporary, false));
  ASSERT_EQ(results_[4], std::make_pair(desktop_capturer::capture_result::error_permanent, false));
  ASSERT_EQ(results_[5], std::make_pair(desktop_capturer::capture_result::error_permanent, false));
}

TEST_F(fallback_desktop_capturer_wrapper_test, with_shared_memory) {
  wrapper_->set_shared_memory_factory(
      std::unique_ptr<shared_memory_factory>(new fake_shared_memory_factory()));
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::error_temporary);
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::success);
  wrapper_->capture_frame();
  main_capturer_->set_result(desktop_capturer::capture_result::error_permanent);
  wrapper_->capture_frame();
  wrapper_->capture_frame();

  ASSERT_EQ(main_capturer_->num_capture_attempts(), 4);
  ASSERT_EQ(main_capturer_->num_frames_captured(), 2);
  ASSERT_EQ(secondary_capturer_->num_capture_attempts(), 3);
  ASSERT_EQ(secondary_capturer_->num_frames_captured(), 3);
  ASSERT_EQ(results_.size(), 5U);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(results_[i], std::make_pair(desktop_capturer::capture_result::success, true));
  }
}

} // namespace base
} // namespace traa
