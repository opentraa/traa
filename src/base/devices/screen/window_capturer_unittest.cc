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

#include "base/checks.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

namespace traa {
namespace base {

class window_capturer_test : public ::testing::Test, public desktop_capturer::capture_callback {
public:
  void SetUp() override {
    capturer_ = desktop_capturer::create_window_capturer(desktop_capture_options::create_default());
    ASSERT_TRUE(capturer_);
  }

  void TearDown() override {}

  // desktop_capturer::capture_callback interface
  void on_capture_result(desktop_capturer::capture_result /* result */,
                         std::unique_ptr<desktop_frame> frame) override {
    frame_ = std::move(frame);
  }

protected:
  std::unique_ptr<desktop_capturer> capturer_;
  std::unique_ptr<desktop_frame> frame_;
};

// Verify that we can enumerate windows.
// TODO(bugs.webrtc.org/12950): Re-enable when libc++ issue is fixed
#if (defined(TRAA_OS_LINUX) && defined(MEMORY_SANITIZER)) ||                                       \
    (defined(TRAA_OS_LINUX) && !defined(TRAA_ENABLE_WAYLAND) && !defined(TRAA_ENABLE_X11))
#define MAYBE_enumerate DISABLED_enumerate
#else
#define MAYBE_enumerate enumerate
#endif
TEST_F(window_capturer_test, MAYBE_enumerate) {
  desktop_capturer::source_list_t sources;
  EXPECT_TRUE(capturer_->get_source_list(&sources));

  // Verify that window titles are set.
  for (auto it = sources.begin(); it != sources.end(); ++it) {
    EXPECT_FALSE(it->title.empty());
  }
}

// Flaky on Linux. See: crbug.com/webrtc/7830.
// Failing on macOS 11: See bugs.webrtc.org/12801
#if defined(TRAA_OS_LINUX) || defined(TRAA_OS_MAC)
#define MAYBE_capture DISABLED_capture
#else
#define MAYBE_capture capture
#endif
// Verify we can capture a window.
//
// TODO(sergeyu): Currently this test just looks at the windows that already
// exist. Ideally it should create a test window and capture from it, but there
// is no easy cross-platform way to create new windows (potentially we could
// have a python script showing Tk dialog, but launching code will differ
// between platforms).
TEST_F(window_capturer_test, MAYBE_capture) {
  desktop_capturer::source_list_t sources;
  capturer_->start(this);
  EXPECT_TRUE(capturer_->get_source_list(&sources));

  // Verify that we can select and capture each window.
  for (auto it = sources.begin(); it != sources.end(); ++it) {
    frame_.reset();
    if (capturer_->select_source(it->id)) {
      capturer_->capture_frame();
    }

    // If we failed to capture a window make sure it no longer exists.
    if (!frame_.get()) {
      desktop_capturer::source_list_t new_list;
      EXPECT_TRUE(capturer_->get_source_list(&new_list));
      for (auto new_list_it = new_list.begin(); new_list_it != new_list.end(); ++new_list_it) {
        EXPECT_FALSE(it->id == new_list_it->id);
      }
      continue;
    }

    EXPECT_GT(frame_->size().width(), 0);
    EXPECT_GT(frame_->size().height(), 0);
  }
}

} // namespace base
} // namespace traa
