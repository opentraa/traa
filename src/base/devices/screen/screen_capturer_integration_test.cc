/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/base64/base64.h"
#include "base/checks.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/rgba_color.h"
#include "base/devices/screen/test/mock_desktop_capturer_callback.h"
#include "base/devices/screen/test/screen_drawer.h"
#include "base/logger.h"

#if defined(TRAA_OS_WINDOWS)
#include "base/devices/screen/win/screen_capturer_win_directx.h"
#include "base/utils/win/version.h"
#endif // defined(TRAA_OS_WINDOWS)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string.h>

#include <algorithm>
#include <initializer_list>
#include <iostream> // TODO(zijiehe): Remove once flaky has been resolved.
#include <memory>
#include <utility>

using ::testing::_;

namespace traa {
namespace base {

namespace {

ACTION_P2(action_save_capture_result, result, dest) {
  *result = arg0;
  *dest = std::move(*arg1);
}

// Returns true if color in `rect` of `frame` is `color`.
bool are_pixels_colored_by(const desktop_frame &frame, desktop_rect rect, rgba_color color,
                           bool may_partially_draw) {
  if (!may_partially_draw) {
    // updated_region() should cover the painted area.
    desktop_region updated_region(frame.updated_region());
    updated_region.intersect_with(rect);
    if (!updated_region.equals(desktop_region(rect))) {
      return false;
    }
  }

  // Color in the `rect` should be `color`.
  uint8_t *row = frame.get_frame_data_at_pos(rect.top_left());
  for (int i = 0; i < rect.height(); i++) {
    uint8_t *column = row;
    for (int j = 0; j < rect.width(); j++) {
      if (color != rgba_color(column)) {
        return false;
      }
      column += desktop_frame::k_bytes_per_pixel;
    }
    row += frame.stride();
  }
  return true;
}

} // namespace

class screen_capturer_integration_test : public ::testing::Test {
public:
  void SetUp() override {
    capturer_ = desktop_capturer::create_screen_capturer(desktop_capture_options::create_default());
  }

protected:
  void test_capture_updated_region(std::initializer_list<desktop_capturer *> capturers) {
    TRAA_DCHECK(capturers.size() > 0);
// A large enough area for the tests, which should be able to be fulfilled
// by most systems.
#if defined(TRAA_OS_WINDOWS)
    // On Windows, an interesting warning window may pop up randomly. The root
    // cause is still under investigation, so reduce the test area to work
    // around. Bug https://bugs.chromium.org/p/webrtc/issues/detail?id=6666.
    const int k_test_area = 416;
#else
    const int k_test_area = 512;
#endif
    const int k_rect_size = 32;
    std::unique_ptr<screen_drawer> drawer = screen_drawer::create();
    if (!drawer || drawer->drawable_region().is_empty()) {
      LOG_WARN("No ScreenDrawer implementation for current platform.");
      return;
    }
    if (drawer->drawable_region().width() < k_test_area ||
        drawer->drawable_region().height() < k_test_area) {
      LOG_WARN("ScreenDrawer::DrawableRegion() is too small for the "
               "CaptureUpdatedRegion tests.");
      return;
    }

    for (desktop_capturer *capturer : capturers) {
      capturer->start(&callback_);
    }

    // Draw a set of `k_rect_size` by `k_rect_size` rectangles at (`i`, `i`), or
    // `i` by `i` rectangles at (`k_rect_size`, `k_rect_size`). One of (controlled
    // by `c`) its primary colors is `i`, and the other two are 0x7f. So we
    // won't draw a black or white rectangle.
    for (int c = 0; c < 3; c++) {
      // A fixed size rectangle.
      for (int i = 0; i < k_test_area - k_rect_size; i += 16) {
        desktop_rect rect = desktop_rect::make_xywh(i, i, k_rect_size, k_rect_size);
        rect.translate(drawer->drawable_region().top_left());
        rgba_color color((c == 0 ? (i & 0xff) : 0x7f), (c == 1 ? (i & 0xff) : 0x7f),
                         (c == 2 ? (i & 0xff) : 0x7f));
        // Fail fast.
        ASSERT_NO_FATAL_FAILURE(test_capture_one_frame(capturers, drawer.get(), rect, color));
      }

      // A variable-size rectangle.
      for (int i = 0; i < k_test_area - k_rect_size; i += 16) {
        desktop_rect rect = desktop_rect::make_xywh(k_rect_size, k_rect_size, i, i);
        rect.translate(drawer->drawable_region().top_left());
        rgba_color color((c == 0 ? (i & 0xff) : 0x7f), (c == 1 ? (i & 0xff) : 0x7f),
                         (c == 2 ? (i & 0xff) : 0x7f));
        // Fail fast.
        ASSERT_NO_FATAL_FAILURE(test_capture_one_frame(capturers, drawer.get(), rect, color));
      }
    }
  }

  void test_capture_updated_region() { test_capture_updated_region({capturer_.get()}); }

#if defined(TRAA_OS_WINDOWS)
  // Enable allow_directx_capturer in desktop_capture_options, but let
  // desktop_capturer::create_screen_capturer() to decide whether a DirectX
  // capturer should be used.
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

private:
  // Repeats capturing the frame by using `capturers` one-by-one for 600 times,
  // typically 30 seconds, until they succeeded captured a `color` rectangle at
  // `rect`. This function uses `drawer`->WaitForPendingDraws() between two
  // attempts to wait for the screen to update.
  void test_capture_one_frame(std::vector<desktop_capturer *> capturers, screen_drawer *drawer,
                              desktop_rect rect, rgba_color color) {
    const int wait_capture_round = 600;
    drawer->clear();
    size_t succeeded_capturers = 0;
    for (int i = 0; i < wait_capture_round; i++) {
      drawer->draw_rectangle(rect, color);
      drawer->wait_for_pending_draws();
      for (size_t j = 0; j < capturers.size(); j++) {
        if (capturers[j] == nullptr) {
          // DesktopCapturer should return an empty updated_region() if no
          // update detected. So we won't test it again if it has captured the
          // rectangle we drew.
          continue;
        }
        std::unique_ptr<desktop_frame> frame = capture_frame(capturers[j]);
        if (!frame) {
          // CaptureFrame() has triggered an assertion failure already, we only
          // need to return here.
          return;
        }

        if (are_pixels_colored_by(*frame, rect, color, drawer->may_draw_incomplete_shapes())) {
          capturers[j] = nullptr;
          succeeded_capturers++;
        }
        // The following else if statement is for debugging purpose only, which
        // should be removed after flaky of screen_capturer_integration_test has
        // been resolved.
        else if (i == wait_capture_round - 1) {
          std::string result;
          base64::encode_from_array(frame->data(), frame->size().height() * frame->stride(),
                                    &result);
          std::cout << frame->size().width() << " x " << frame->size().height() << std::endl;
          // Split the entire string (can be over 4M) into several lines to
          // avoid browser from sticking.
          static const size_t kLineLength = 32768;
          const char *result_end = result.c_str() + result.length();
          for (const char *it = result.c_str(); it < result_end; it += kLineLength) {
            const size_t max_length = result_end - it;
            std::cout << std::string(it, std::min(kLineLength, max_length)) << std::endl;
          }
          std::cout << "Failed to capture rectangle " << rect.left() << " x " << rect.top() << " - "
                    << rect.right() << " x " << rect.bottom() << " with color ("
                    << static_cast<int>(color.red) << ", " << static_cast<int>(color.green) << ", "
                    << static_cast<int>(color.blue) << ", " << static_cast<int>(color.alpha) << ")"
                    << std::endl;
          ASSERT_TRUE(false) << "screen_capturer_integration_test may be flaky. "
                                "Please kindly FYI the broken link to "
                                "zijiehe@chromium.org for investigation. If "
                                "the failure continually happens, but I have "
                                "not responded as quick as expected, disable "
                                "*all* tests in "
                                "screen_capturer_integration_test.cc to "
                                "unblock other developers.";
        }
      }

      if (succeeded_capturers == capturers.size()) {
        break;
      }
    }

    ASSERT_EQ(succeeded_capturers, capturers.size());
  }

  // Expects `capturer` to successfully capture a frame, and returns it.
  std::unique_ptr<desktop_frame> capture_frame(desktop_capturer *capturer) {
    for (int i = 0; i < 10; i++) {
      std::unique_ptr<desktop_frame> frame;
      desktop_capturer::capture_result result;
      EXPECT_CALL(callback_, on_capture_result_ptr(_, _))
          .WillOnce(action_save_capture_result(&result, &frame));
      capturer->capture_frame();
      ::testing::Mock::VerifyAndClearExpectations(&callback_);
      if (result == desktop_capturer::capture_result::success) {
        EXPECT_TRUE(frame);
        return frame;
      } else {
        EXPECT_FALSE(frame);
      }
    }

    EXPECT_TRUE(false);
    return nullptr;
  }
};

#if defined(TRAA_OS_WINDOWS)
// ScreenCapturerWinGdi randomly returns blank screen, the root cause is still
// unknown. Bug, https://bugs.chromium.org/p/webrtc/issues/detail?id=6843.
#define MAYBE_capture_updated_region DISABLED_capture_updated_region
#else
#define MAYBE_capture_updated_region capture_updated_region
#endif
TEST_F(screen_capturer_integration_test, MAYBE_capture_updated_region) {
  test_capture_updated_region();
}

#if defined(TRAA_OS_WINDOWS)
// ScreenCapturerWinGdi randomly returns blank screen, the root cause is still
// unknown. Bug, https://bugs.chromium.org/p/webrtc/issues/detail?id=6843.
#define MAYBE_two_capturers DISABLED_two_capturers
#else
#define MAYBE_two_capturers two_capturers
#endif
TEST_F(screen_capturer_integration_test, MAYBE_two_capturers) {
  std::unique_ptr<desktop_capturer> capturer2 = std::move(capturer_);
  SetUp();
  test_capture_updated_region({capturer_.get(), capturer2.get()});
}

#if defined(TRAA_OS_WINDOWS)

// Windows cannot capture contents on VMs hosted in GCE. See bug
// https://bugs.chromium.org/p/webrtc/issues/detail?id=8153.
TEST_F(screen_capturer_integration_test, DISABLED_capture_updated_region_with_directx_capturer) {
  if (!create_directx_capturer()) {
    return;
  }

  test_capture_updated_region();
}

TEST_F(screen_capturer_integration_test, DISABLED_two_directx_capturers) {
  if (!create_directx_capturer()) {
    return;
  }

  std::unique_ptr<desktop_capturer> capturer2 = std::move(capturer_);
  TRAA_CHECK(create_directx_capturer());
  test_capture_updated_region({capturer_.get(), capturer2.get()});
}

TEST_F(screen_capturer_integration_test,
       DISABLED_maybe_capture_updated_region_with_directx_capturer) {
  if (os_get_version() < version_alias::VERSION_WIN8) {
    // ScreenCapturerWinGdi randomly returns blank screen, the root cause is
    // still unknown. Bug,
    // https://bugs.chromium.org/p/webrtc/issues/detail?id=6843.
    // On Windows 7 or early version, maybe_create_directx_capturer() always
    // creates GDI capturer.
    return;
  }
  // Even DirectX capturer is not supported in current system, we should be able
  // to select a usable capturer.
  maybe_create_directx_capturer();
  test_capture_updated_region();
}

#endif // defined(TRAA_OS_WINDOWS)

} // namespace base
} // namespace traa
