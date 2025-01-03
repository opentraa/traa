/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer.h"

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/test/mock_desktop_capturer_callback.h"
#include "base/logger.h"

#if defined(TRAA_OS_WINDOWS)
#include "base/devices/screen/win/screen_capturer_win_directx.h"
#endif // defined(TRAA_OS_WINDOWS)

#include <gtest/gtest.h>

#include <memory>

using ::testing::_;

const int k_test_shared_memory_id = 123;

namespace traa {
namespace base {

class screen_capturer_test : public ::testing::Test {
public:
  void SetUp() override {
#if defined(TRAA_OS_WINDOWS)
    ::SetProcessDPIAware();
#endif
    capturer_ = desktop_capturer::create_screen_capturer(desktop_capture_options::create_default());
#if defined(TRAA_OS_WINDOWS) || defined(TRAA_OS_MACOS) ||                                          \
    (defined(TRAA_OS_LINUX) && (defined(TRAA_ENABLE_WAYLAND) || defined(TRAA_ENABLE_X11)))
    ASSERT_TRUE(capturer_);
#endif
  }

protected:
#if defined(TRAA_OS_WINDOWS)
  // Enable allow_directx_capturer in DesktopCaptureOptions, but let
  // DesktopCapturer::CreateScreenCapturer decide whether a DirectX capturer
  // should be used.
  void maybe_create_directx_capturer() {
    desktop_capture_options options(desktop_capture_options::create_default());
    options.set_allow_directx_capturer(true);
    capturer_ = desktop_capturer::create_screen_capturer(options);
  }

  bool create_directx_capturer() {
    if (!screen_capturer_win_directx::is_supported()) {
      LOG_WARN("directx capturer is not supported");
      return false;
    }

    maybe_create_directx_capturer();
    return true;
  }
#endif // defined(TRAA_OS_WINDOWS)

  std::unique_ptr<desktop_capturer> capturer_;
  mock_desktop_capturer_callback callback_;
};

class fake_shared_memory : public shared_memory {
public:
  fake_shared_memory(char *buffer, size_t size)
      : shared_memory(buffer, size, 0, k_test_shared_memory_id), buffer_(buffer) {}
  ~fake_shared_memory() override { delete[] buffer_; }

  fake_shared_memory(const fake_shared_memory &) = delete;
  fake_shared_memory &operator=(const fake_shared_memory &) = delete;

private:
  char *buffer_;
};

class fake_shared_memory_factory : public shared_memory_factory {
public:
  fake_shared_memory_factory() {}
  ~fake_shared_memory_factory() override {}

  fake_shared_memory_factory(const fake_shared_memory_factory &) = delete;
  fake_shared_memory_factory &operator=(const fake_shared_memory_factory &) = delete;

  std::unique_ptr<shared_memory> create_shared_memory(size_t size) override {
    return std::unique_ptr<shared_memory>(new fake_shared_memory(new char[size], size));
  }
};

ACTION_P(action_save_unique_ptr_arg, dest) { *dest = std::move(*arg1); }

// TODO(bugs.webrtc.org/12950): Re-enable when libc++ issue is fixed.
#if (defined(TRAA_OS_LINUX) && defined(MEMORY_SANITIZER)) ||                                       \
    (defined(TRAA_OS_LINUX) && !defined(TRAA_ENABLE_WAYLAND) && !defined(TRAA_ENABLE_X11))
#define MAYBE_get_screen_list_and_select_screen DISABLED_get_screen_list_and_select_screen
#else
#define MAYBE_get_screen_list_and_select_screen get_screen_list_and_select_screen
#endif
TEST_F(screen_capturer_test, MAYBE_get_screen_list_and_select_screen) {
  desktop_capturer::source_list_t screens;
  EXPECT_TRUE(capturer_->get_source_list(&screens));
  for (const auto &screen : screens) {
    EXPECT_TRUE(capturer_->select_source(screen.id));
  }
}

// Flaky on Linux. See: crbug.com/webrtc/7830
#if defined(TRAA_OS_LINUX)
#define MAYBE_start_capturer DISABLED_start_capturer
#else
#define MAYBE_start_capturer start_capturer
#endif
TEST_F(screen_capturer_test, MAYBE_start_capturer) { capturer_->start(&callback_); }

#if defined(TRAA_OS_LINUX)
#define MAYBE_capture DISABLED_capture
#else
#define MAYBE_capture capture
#endif
TEST_F(screen_capturer_test, MAYBE_capture) {
  // Assume that Start() treats the screen as invalid initially.
  std::unique_ptr<desktop_frame> frame;
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .WillOnce(action_save_unique_ptr_arg(&frame));

  capturer_->start(&callback_);
  capturer_->capture_frame();

  ASSERT_TRUE(frame);
  EXPECT_GT(frame->size().width(), 0);
  EXPECT_GT(frame->size().height(), 0);
  EXPECT_GE(frame->stride(), frame->size().width() * desktop_frame::k_bytes_per_pixel);
  EXPECT_TRUE(frame->get_shared_memory() == nullptr);

  // Verify that the region contains whole screen.
  EXPECT_FALSE(frame->updated_region().is_empty());
  desktop_region::iterator it(frame->updated_region());
  ASSERT_TRUE(!it.is_at_end());
  EXPECT_TRUE(it.rect().equals(desktop_rect::make_size(frame->size())));
  it.advance();
  EXPECT_TRUE(it.is_at_end());
}

#if defined(TRAA_OS_WINDOWS)

TEST_F(screen_capturer_test, use_shared_buffers) {
  std::unique_ptr<desktop_frame> frame;
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .WillOnce(action_save_unique_ptr_arg(&frame));

  capturer_->start(&callback_);
  capturer_->set_shared_memory_factory(
      std::unique_ptr<shared_memory_factory>(new fake_shared_memory_factory()));
  capturer_->capture_frame();

  ASSERT_TRUE(frame);
  ASSERT_TRUE(frame->get_shared_memory());
  EXPECT_EQ(frame->get_shared_memory()->id(), k_test_shared_memory_id);
}

TEST_F(screen_capturer_test, gdi_is_default) {
  std::unique_ptr<desktop_frame> frame;
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .WillOnce(action_save_unique_ptr_arg(&frame));

  capturer_->start(&callback_);
  capturer_->capture_frame();
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->get_capturer_id(), desktop_capture_id::create_four_cc('G', 'D', 'I', ' '));
}

TEST_F(screen_capturer_test, use_directx_capturer) {
  if (!create_directx_capturer()) {
    return;
  }

  std::unique_ptr<desktop_frame> frame;
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .WillOnce(action_save_unique_ptr_arg(&frame));

  capturer_->start(&callback_);
  capturer_->capture_frame();
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->get_capturer_id(), desktop_capture_id::k_capture_dxgi);
}

TEST_F(screen_capturer_test, use_directx_capturer_with_shared_buffers) {
  if (!create_directx_capturer()) {
    return;
  }

  std::unique_ptr<desktop_frame> frame;
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .WillOnce(action_save_unique_ptr_arg(&frame));

  capturer_->start(&callback_);
  capturer_->set_shared_memory_factory(
      std::unique_ptr<shared_memory_factory>(new fake_shared_memory_factory()));
  capturer_->capture_frame();
  ASSERT_TRUE(frame);
  ASSERT_TRUE(frame->get_shared_memory());
  EXPECT_EQ(frame->get_shared_memory()->id(), k_test_shared_memory_id);
  EXPECT_EQ(frame->get_capturer_id(), desktop_capture_id::k_capture_dxgi);
}

#endif // defined(TRAA_OS_WINDOWS)

} // namespace base
} // namespace traa
