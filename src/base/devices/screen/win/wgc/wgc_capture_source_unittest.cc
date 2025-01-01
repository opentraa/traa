/*
 *  Copyright (c) 2021 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_capture_source.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/test/win/test_window.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/wgc/wgc_capturer_win.h"
#include "base/logger.h"
#include "base/utils/win/scoped_com_initializer.h"

#include <windows.graphics.capture.h>
#include <wrl/client.h>

#include <utility>

#include <gtest/gtest.h>

namespace traa {
namespace base {

inline namespace {

const WCHAR k_window_title[] = L"WGC Capture Source Test Window";

const int k_first_x_coord = 25;
const int k_first_y_coord = 50;
const int k_second_x_coord = 50;
const int k_second_y_coord = 75;

} // namespace

class wgc_capture_source_test : public ::testing::TestWithParam<capture_type> {
public:
  void SetUp() override {
    com_initializer_ = std::make_unique<scoped_com_initializer>(scoped_com_initializer::SELECT_MTA);
    ASSERT_TRUE(com_initializer_->succeeded());
  }

  void TearDown() override {
    if (window_open_) {
      destroy_test_window(window_info_);
    }
  }

  void set_up_for_window_source() {
    window_info_ = create_test_window(k_window_title);
    window_open_ = true;
    source_id_ = reinterpret_cast<desktop_capturer::source_id_t>(window_info_.hwnd);
    source_factory_ = std::make_unique<wgc_window_source_factory>();
  }

  void set_up_for_screen_source() {
    source_id_ = k_screen_id_full;
    source_factory_ = std::make_unique<wgc_screen_source_factory>();
  }

protected:
  std::unique_ptr<scoped_com_initializer> com_initializer_;
  std::unique_ptr<wgc_capture_source_factory> source_factory_;
  std::unique_ptr<wgc_capture_source> source_;
  desktop_capturer::source_id_t source_id_;
  window_info window_info_;
  bool window_open_ = false;
};

// Window specific test
TEST_F(wgc_capture_source_test, windows_position) {
  if (!wgc_capturer_win::is_wgc_supported(capture_type::window)) {
    LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
    GTEST_SKIP();
  }

  set_up_for_window_source();
  source_ = source_factory_->create_capture_source(source_id_);
  ASSERT_TRUE(source_);
  EXPECT_EQ(source_->get_source_id(), source_id_);

  move_test_window(window_info_.hwnd, k_first_x_coord, k_first_y_coord);
  desktop_vector source_vector = source_->get_top_left();
  EXPECT_EQ(source_vector.x(), k_first_x_coord);
  EXPECT_EQ(source_vector.y(), k_first_y_coord);

  move_test_window(window_info_.hwnd, k_second_x_coord, k_second_y_coord);
  source_vector = source_->get_top_left();
  EXPECT_EQ(source_vector.x(), k_second_x_coord);
  EXPECT_EQ(source_vector.y(), k_second_y_coord);
}

// Screen specific test
TEST_F(wgc_capture_source_test, screen_position) {
  if (!wgc_capturer_win::is_wgc_supported(capture_type::screen)) {
    LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
    GTEST_SKIP();
  }

  set_up_for_screen_source();
  source_ = source_factory_->create_capture_source(source_id_);
  ASSERT_TRUE(source_);
  EXPECT_EQ(source_id_, source_->get_source_id());

  desktop_rect screen_rect = capture_utils::get_full_screen_rect();
  desktop_vector source_vector = source_->get_top_left();
  EXPECT_EQ(source_vector.x(), screen_rect.left());
  EXPECT_EQ(source_vector.y(), screen_rect.top());
}

// Source agnostic test
TEST_P(wgc_capture_source_test, create_source) {
  if (!wgc_capturer_win::is_wgc_supported(GetParam())) {
    LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
    GTEST_SKIP();
  }

  if (GetParam() == capture_type::window) {
    set_up_for_window_source();
  } else {
    set_up_for_screen_source();
  }

  source_ = source_factory_->create_capture_source(source_id_);
  ASSERT_TRUE(source_);
  EXPECT_EQ(source_id_, source_->get_source_id());
  EXPECT_TRUE(source_->should_be_capturable());

  Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> item;
  EXPECT_TRUE(SUCCEEDED(source_->get_capture_item(&item)));
  EXPECT_TRUE(item);
}

INSTANTIATE_TEST_SUITE_P(source_agnostic, wgc_capture_source_test,
                         ::testing::Values(capture_type::window, capture_type::screen));

} // namespace base
} // namespace traa
