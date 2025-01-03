/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/test/mock_desktop_capturer_callback.h"

#include <gtest/gtest.h>

#include <ApplicationServices/ApplicationServices.h>

#include <memory>
#include <ostream>

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;

namespace traa {
namespace base {

class screen_capturer_mac_test : public ::testing::Test {
public:
  // Verifies that the whole screen is initially dirty.
  void capture_done_callback1(desktop_capturer::capture_result result,
                              std::unique_ptr<desktop_frame> *frame);

  // Verifies that a rectangle explicitly marked as dirty is propagated
  // correctly.
  void capture_done_callback2(desktop_capturer::capture_result result,
                              std::unique_ptr<desktop_frame> *frame);

protected:
  void SetUp() override {
    capturer_ = desktop_capturer::create_screen_capturer(desktop_capture_options::create_default());
  }

  std::unique_ptr<desktop_capturer> capturer_;
  mock_desktop_capturer_callback callback_;
};

void screen_capturer_mac_test::capture_done_callback1(desktop_capturer::capture_result result,
                                                      std::unique_ptr<desktop_frame> *frame) {
  EXPECT_EQ(result, desktop_capturer::capture_result::success);

  desktop_configuration config = desktop_configuration::current(
      desktop_configuration::coordinate_origin::COORDINATE_BOTTOM_LEFT);

  // Verify that the region size is the same as the pixel bounds.
  desktop_region::iterator it((*frame)->updated_region());
    
  // can not use equals, coz the pixel bounds in desktop configuration is in the virtual
  // screen coordinate system, which may not start from (0, 0), so we need to check
  // if the region size is the same as the pixel bounds.
  EXPECT_TRUE(!it.is_at_end() && it.rect().size().equals(config.pixel_bounds.size()));
}

void screen_capturer_mac_test::capture_done_callback2(desktop_capturer::capture_result result,
                                                      std::unique_ptr<desktop_frame> *frame) {
  EXPECT_EQ(result, desktop_capturer::capture_result::success);

  desktop_configuration config = desktop_configuration::current(
      desktop_configuration::coordinate_origin::COORDINATE_BOTTOM_LEFT);
  int width = config.pixel_bounds.width();
  int height = config.pixel_bounds.height();

  EXPECT_EQ(width, (*frame)->size().width());
  EXPECT_EQ(height, (*frame)->size().height());
  EXPECT_TRUE((*frame)->data() != NULL);
  // Depending on the capture method, the screen may be flipped or not, so
  // the stride may be positive or negative.
  EXPECT_EQ(static_cast<int>(sizeof(uint32_t) * width), abs((*frame)->stride()));
}

TEST_F(screen_capturer_mac_test, capture) {
  EXPECT_CALL(callback_, on_capture_result_ptr(desktop_capturer::capture_result::success, _))
      .Times(2)
      .WillOnce(Invoke(this, &screen_capturer_mac_test::capture_done_callback1))
      .WillOnce(Invoke(this, &screen_capturer_mac_test::capture_done_callback2));

  SCOPED_TRACE("");
  capturer_->start(&callback_);

  // Check that we get an initial full-screen updated.
  capturer_->capture_frame();

  // Check that subsequent dirty rects are propagated correctly.
  capturer_->capture_frame();
}

} // namespace base
} // namespace traa
