/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_capturer_win.h"

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/test/win/test_window.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/wgc/wgc_capture_session.h"
#include "base/logger.h"
#include "base/system/sleep.h"
#include "base/utils/time_utils.h"
#include "base/utils/win/scoped_com_initializer.h"
#include "base/utils/win/version.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

// TODO @sylar: we need to implement the thread and task queue to run the test.
// see more details in "rtc_base/task_queue_for_test.h" and "rtc_base/thread.h".
#if defined(TRAA_ENABLE_WGC_CAPTURER_TEST)

namespace traa {
namespace base {

inline namespace {
constexpr char k_window_thread_name[] = "wgc_capturer_test_window_thread";
constexpr WCHAR k_window_title[] = L"WGC Capturer Test Window";

constexpr char k_capturer_impl_histogram[] = "WebRTC.DesktopCapture.Win.DesktopCapturerImpl";

constexpr char k_capturer_result_histogram[] = "WebRTC.DesktopCapture.Win.WgcCapturerResult";
constexpr int k_success = 0;
constexpr int k_session_start_failure = 4;

constexpr char k_capture_session_result_histogram[] =
    "WebRTC.DesktopCapture.Win.WgcCaptureSessionStartResult";
constexpr int k_source_closed = 1;

constexpr char k_capture_time_histogram[] = "WebRTC.DesktopCapture.Win.WgcCapturerFrameTime";

// The capturer keeps `kNumBuffers` in its frame pool, so we need to request
// that many frames to clear those out. The next frame will have the new size
// (if the size has changed) so we will resize the frame pool at this point.
// Then, we need to clear any frames that may have delivered to the frame pool
// before the resize. Finally, the next frame will be guaranteed to be the new
// size.
constexpr int k_num_captures_to_flush_buffers = wgc_capture_session::k_num_buffers * 2 + 1;

constexpr int k_small_window_width = 200;
constexpr int k_small_window_height = 100;
constexpr int k_medium_window_width = 300;
constexpr int k_medium_window_height = 200;
constexpr int k_large_window_width = 400;
constexpr int k_large_window_height = 500;

// The size of the image we capture is slightly smaller than the actual size of
// the window.
constexpr int k_window_width_subtrahend = 14;
constexpr int k_window_height_subtrahend = 7;

// Custom message constants so we can direct our thread to close windows and
// quit running.
constexpr UINT k_destroy_window = WM_APP;
constexpr UINT k_quit_running = WM_APP + 1;

// When testing changes to real windows, sometimes the effects (close or resize)
// don't happen immediately, we want to keep trying until we see the effect but
// only for a reasonable amount of time.
constexpr int k_max_tries = 50;

} // namespace

class wgc_capturer_win_test : public ::testing::TestWithParam<capture_type>,
                              public desktop_capturer::capture_callback {
public:
  void SetUp() override {
    com_initializer_ = std::make_unique<scoped_com_initializer>(scoped_com_initializer::SELECT_MTA);
    EXPECT_TRUE(com_initializer_->succeeded());

    if (!wgc_capturer_win::is_wgc_supported(GetParam())) {
      LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
      GTEST_SKIP();
    }
  }

  void SetUpForWindowCapture(int window_width = k_medium_window_width,
                             int window_height = k_medium_window_height) {
    capturer_ =
        wgc_capturer_win::create_raw_window_capturer(desktop_capture_options::create_default());
    create_window_on_separate_thread(window_width, window_height);
    start_window_thread_message_loop();
    source_id_ = get_test_window_id_from_source_list();
  }

  void SetUpForScreenCapture() {
    capturer_ =
        wgc_capturer_win::create_raw_screen_capturer(desktop_capture_options::create_default());
    source_id_ = get_screen_id_from_source_list();
  }

  void TearDown() override {
    if (window_open_) {
      close_test_window();
    }
  }

  // The window must live on a separate thread so that we can run a message pump
  // without blocking the test thread. This is necessary if we are interested in
  // having GraphicsCaptureItem events (i.e. the Closed event) fire, and it more
  // closely resembles how capture works in the wild.
  void create_window_on_separate_thread(int window_width, int window_height) {
    window_thread_ = rtc::thread::create();
    window_thread_->set_name(k_window_thread_name, nullptr);
    window_thread_->start();
    send_task(window_thread_.get(), [this, window_width, window_height]() {
      window_thread_id_ = GetCurrentThreadId();
      window_info_ = create_test_window(k_window_title, window_height, window_width);
      window_open_ = true;

      while (!capture_utils::is_window_response(window_info_.hwnd)) {
        LOG_INFO("Waiting for test window to become responsive in "
                 "WgcWindowCaptureTest.");
      }

      while (!capture_utils::is_window_valid_and_visible(window_info_.hwnd)) {
        LOG_INFO("Waiting for test window to be visible in "
                 "WgcWindowCaptureTest.");
      }
    });

    ASSERT_TRUE(window_thread_->RunningForTest());
    ASSERT_FALSE(window_thread_->IsCurrent());
  }

  void StartWindowThreadMessageLoop() {
    window_thread_->PostTask([this]() {
      MSG msg;
      BOOL gm;
      while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
        ::DispatchMessage(&msg);
        if (msg.message == k_destroy_window) {
          destroy_test_window(window_info_);
        }
        if (msg.message == k_quit_running) {
          post_quit_message(0);
        }
      }
    });
  }

  void close_test_window() {
    ::PostThreadMessage(window_thread_id_, k_destroy_window, 0, 0);
    ::PostThreadMessage(window_thread_id_, k_quit_running, 0, 0);
    window_thread_->Stop();
    window_open_ = false;
  }

  desktop_capturer::source_id_t get_test_window_id_from_source_list() {
    // Frequently, the test window will not show up in GetSourceList because it
    // was created too recently. Since we are confident the window will be found
    // eventually we loop here until we find it.
    intptr_t src_id = 0;
    do {
      desktop_capturer::source_list_t sources;
      EXPECT_TRUE(capturer_->get_source_list(&sources));
      auto it =
          std::find_if(sources.begin(), sources.end(), [&](const desktop_capturer::source_t_ &src) {
            return src.id == reinterpret_cast<intptr_t>(window_info_.hwnd);
          });

      if (it != sources.end())
        src_id = it->id;
    } while (src_id != reinterpret_cast<intptr_t>(window_info_.hwnd));

    return src_id;
  }

  desktop_capturer::source_id_t get_screen_id_from_source_list() {
    desktop_capturer::source_list_t sources;
    EXPECT_TRUE(capturer_->get_source_list(&sources));
    EXPECT_GT(sources.size(), 0ULL);
    return sources[0].id;
  }

  void DoCapture(int num_captures = 1) {
    // Capture the requested number of frames. We expect the first capture to
    // always succeed. If we're asked for multiple frames, we do expect to see a
    // a couple dropped frames due to resizing the window.
    const int max_tries = num_captures == 1 ? 1 : k_max_tries;
    int success_count = 0;
    for (int i = 0; success_count < num_captures && i < max_tries; i++) {
      capturer_->capture_frame();
      if (result_ == desktop_capturer::capture_result::error_permanent)
        break;
      if (result_ == desktop_capturer::capture_result::success)
        success_count++;
    }

    total_successful_captures_ += success_count;
    EXPECT_EQ(success_count, num_captures);
    EXPECT_EQ(result_, desktop_capturer::capture_result::success);
    EXPECT_TRUE(frame_);
    EXPECT_GE(metrics::NumEvents(k_capturer_result_histogram, k_success),
              total_successful_captures_);
  }

  void ValidateFrame(int expected_width, int expected_height) {
    EXPECT_EQ(frame_->size().width(), expected_width - k_window_width_subtrahend);
    EXPECT_EQ(frame_->size().height(), expected_height - k_window_height_subtrahend);

    // Verify the buffer contains as much data as it should.
    int data_length = frame_->stride() * frame_->size().height();

    // The first and last pixel should have the same color because they will be
    // from the border of the window.
    // Pixels have 4 bytes of data so the whole pixel needs a uint32_t to fit.
    uint32_t first_pixel = static_cast<uint32_t>(*frame_->data());
    uint32_t last_pixel =
        static_cast<uint32_t>(*(frame_->data() + data_length - desktop_frame::k_bytes_per_pixel));
    EXPECT_EQ(first_pixel, last_pixel);

    // Let's also check a pixel from the middle of the content area, which the
    // test window will paint a consistent color for us to verify.
    uint8_t *middle_pixel = frame_->data() + (data_length / 2);

    int sub_pixel_offset = desktop_frame::k_bytes_per_pixel / 4;
    EXPECT_EQ(*middle_pixel, k_test_window_b_value);
    middle_pixel += sub_pixel_offset;
    EXPECT_EQ(*middle_pixel, k_test_window_g_value);
    middle_pixel += sub_pixel_offset;
    EXPECT_EQ(*middle_pixel, k_test_window_r_value);
    middle_pixel += sub_pixel_offset;

    // The window is opaque so we expect 0xFF for the Alpha channel.
    EXPECT_EQ(*middle_pixel, 0xFF);
  }

  // DesktopCapturer::Callback interface
  // The capturer synchronously invokes this method before `CaptureFrame()`
  // returns.
  void on_capture_result(desktop_capturer::capture_result result,
                         std::unique_ptr<desktop_frame> frame) override {
    result_ = result;
    frame_ = std::move(frame);
  }

protected:
  std::unique_ptr<scoped_com_initializer> com_initializer_;
  DWORD window_thread_id_;
  std::unique_ptr<rtc::thread> window_thread_;
  window_info window_info_;
  intptr_t source_id_;
  bool window_open_ = false;
  desktop_capturer::capture_result result_;
  int total_successful_captures_ = 0;
  std::unique_ptr<desktop_frame> frame_;
  std::unique_ptr<desktop_capturer> capturer_;
};

TEST_P(wgc_capturer_win_test, select_valid_source) {
  if (GetParam() == capture_type::window) {
    SetUpForWindowCapture();
  } else {
    SetUpForScreenCapture();
  }

  EXPECT_TRUE(capturer_->select_source(source_id_));
}

TEST_P(wgc_capturer_win_test, SelectInvalidSource) {
  if (GetParam() == capture_type::window) {
    capturer_ =
        wgc_capturer_win::create_raw_window_capturer(desktop_capture_options::create_default());
    source_id_ = k_window_id_null;
  } else {
    capturer_ =
        wgc_capturer_win::create_raw_screen_capturer(desktop_capture_options::create_default());
    source_id_ = k_screen_id_invalid;
  }

  EXPECT_FALSE(capturer_->select_source(source_id_));
}

TEST_P(wgc_capturer_win_test, Capture) {
  if (GetParam() == capture_type::window) {
    set_up_for_window_capture();
  } else {
    set_up_for_screen_capture();
  }

  EXPECT_TRUE(capturer_->select_source(source_id_));

  capturer_->start(this);
  EXPECT_GE(metrics::NumEvents(k_capturer_impl_histogram, desktop_capture_id::k_capture_wgc), 1);

  DoCapture();
  EXPECT_GT(frame_->size().width(), 0);
  EXPECT_GT(frame_->size().height(), 0);
}

TEST_P(wgc_capturer_win_test, CaptureTime) {
  if (GetParam() == capture_type::window) {
    set_up_for_window_capture();
  } else {
    set_up_for_screen_capture();
  }

  EXPECT_TRUE(capturer_->select_source(source_id_));
  capturer_->start(this);

  int64_t start_time;
  start_time = time_nanos();
  capturer_->capture_frame();

  int capture_time_ms = (time_nanos() - start_time) / k_num_nanosecs_per_millisec;
  EXPECT_EQ(result_, desktop_capturer::capture_result::success);
  EXPECT_TRUE(frame_);

  // The test may measure the time slightly differently than the capturer. So we
  // just check if it's within 5 ms.
  EXPECT_NEAR(frame_->capture_time_ms(), capture_time_ms, 5);
  EXPECT_GE(metrics::NumEvents(k_capture_time_histogram, frame_->capture_time_ms()), 1);
}

INSTANTIATE_TEST_SUITE_P(SourceAgnostic, wgc_capturer_win_test,
                         ::testing::Values(CaptureType::kWindow, CaptureType::kScreen));

TEST(WgcCapturerNoMonitorTest, NoMonitors) {
  scoped_com_initializer com_initializer(scoped_com_initializer::SELECT_MTA);
  EXPECT_TRUE(com_initializer.succeeded());
  if (capture_utils::has_active_display()) {
    LOG_INFO("Skip wgc_capturer_win_test designed specifically for "
             "systems with no monitors");
    GTEST_SKIP();
  }

  // A bug in `CreateForMonitor` prevents screen capture when no displays are
  // attached.
  EXPECT_FALSE(wgc_capturer_win::is_wgc_supported(capture_type::screen));

  // A bug in the DWM (Desktop Window Manager) prevents it from providing image
  // data if there are no displays attached. This was fixed in Windows 11.
  if (os_get_version() < version_alias::VERSION_WIN11)
    EXPECT_FALSE(wgc_capturer_win::is_wgc_supported(capture_type::window));
  else
    EXPECT_TRUE(wgc_capturer_win::is_wgc_supported(capture_type::window));
}

class WgcCapturerMonitorTest : public wgc_capturer_win_test {
public:
  void SetUp() {
    com_initializer_ = std::make_unique<scoped_com_initializer>(scoped_com_initializer::SELECT_MTA);
    EXPECT_TRUE(com_initializer_->succeeded());

    if (!wgc_capturer_win::is_wgc_supported(capture_type::screen)) {
      LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
      GTEST_SKIP();
    }
  }
};

TEST_F(WgcCapturerMonitorTest, FocusOnMonitor) {
  SetUpForScreenCapture();
  EXPECT_TRUE(capturer_->select_source(0));

  // You can't set focus on a monitor.
  EXPECT_FALSE(capturer_->focus_on_selected_source());
}

TEST_F(WgcCapturerMonitorTest, CaptureAllMonitors) {
  SetUpForScreenCapture();
  EXPECT_TRUE(capturer_->select_source(k_screen_id_full));

  capturer_->start(this);
  DoCapture();
  EXPECT_GT(frame_->size().width(), 0);
  EXPECT_GT(frame_->size().height(), 0);
}

class WgcCapturerWindowTest : public wgc_capturer_win_test {
public:
  void SetUp() {
    com_initializer_ = std::make_unique<scoped_com_initializer>(scoped_com_initializer::SELECT_MTA);
    EXPECT_TRUE(com_initializer_->succeeded());

    if (!wgc_capturer_win::is_wgc_supported(capture_type::window)) {
      LOG_INFO("Skipping WgcCapturerWinTests on unsupported platforms.");
      GTEST_SKIP();
    }
  }
};

TEST_F(WgcCapturerWindowTest, FocusOnWindow) {
  capturer_ =
      wgc_capturer_win::create_raw_window_capturer(desktop_capture_options::create_default());
  window_info_ = create_test_window(k_window_title);
  source_id_ = get_screen_id_from_source_list();

  EXPECT_TRUE(capturer_->select_source(source_id_));
  EXPECT_TRUE(capturer_->focus_on_selected_source());

  HWND hwnd = reinterpret_cast<HWND>(source_id_);
  EXPECT_EQ(hwnd, ::GetActiveWindow());
  EXPECT_EQ(hwnd, ::GetForegroundWindow());
  EXPECT_EQ(hwnd, ::GetFocus());
  destroy_test_window(window_info_);
}

TEST_F(WgcCapturerWindowTest, SelectMinimizedWindow) {
  SetUpForWindowCapture();
  minimize_test_window(reinterpret_cast<HWND>(source_id_));
  EXPECT_FALSE(capturer_->select_source(source_id_));

  unminimize_test_window(reinterpret_cast<HWND>(source_id_));
  EXPECT_TRUE(capturer_->select_source(source_id_));
}

TEST_F(WgcCapturerWindowTest, SelectClosedWindow) {
  SetUpForWindowCapture();
  EXPECT_TRUE(capturer_->select_source(source_id_));

  close_test_window();
  EXPECT_FALSE(capturer_->select_source(source_id_));
}

TEST_F(WgcCapturerWindowTest, UnsupportedWindowStyle) {
  // Create a window with the WS_EX_TOOLWINDOW style, which WGC does not
  // support.
  window_info_ = create_test_window(k_window_title, k_medium_window_width, k_medium_window_height,
                                    WS_EX_TOOLWINDOW);
  capturer_ =
      wgc_capturer_win::create_raw_window_capturer(desktop_capture_options::create_default());
  desktop_capturer::source_list_t sources;
  EXPECT_TRUE(capturer_->get_source_list(&sources));
  auto it =
      std::find_if(sources.begin(), sources.end(), [&](const desktop_capturer::source_t_ &src) {
        return src.id == reinterpret_cast<intptr_t>(window_info_.hwnd);
      });

  // We should not find the window, since we filter for unsupported styles.
  EXPECT_EQ(it, sources.end());
  destroy_test_window(window_info_);
}

TEST_F(WgcCapturerWindowTest, IncreaseWindowSizeMidCapture) {
  SetUpForWindowCapture(k_small_window_width, k_small_window_height);
  EXPECT_TRUE(capturer_->select_source(source_id_));

  capturer_->start(this);
  DoCapture();
  ValidateFrame(k_small_window_width, k_small_window_height);

  resize_test_window(window_info_.hwnd, k_small_window_width, k_medium_window_height);
  DoCapture(k_num_captures_to_flush_buffers);
  ValidateFrame(k_small_window_width, k_medium_window_height);

  resize_test_window(window_info_.hwnd, k_large_window_width, k_medium_window_height);
  DoCapture(k_num_captures_to_flush_buffers);
  ValidateFrame(k_large_window_width, k_medium_window_height);
}

TEST_F(WgcCapturerWindowTest, ReduceWindowSizeMidCapture) {
  SetUpForWindowCapture(k_large_window_width, k_large_window_height);
  EXPECT_TRUE(capturer_->select_source(source_id_));

  capturer_->start(this);
  DoCapture();
  ValidateFrame(k_large_window_width, k_large_window_height);

  resize_test_window(window_info_.hwnd, k_large_window_width, k_medium_window_height);
  DoCapture(k_num_captures_to_flush_buffers);
  ValidateFrame(k_large_window_width, k_medium_window_height);

  resize_test_window(window_info_.hwnd, k_small_window_width, k_medium_window_height);
  DoCapture(k_num_captures_to_flush_buffers);
  ValidateFrame(k_small_window_width, k_medium_window_height);
}

TEST_F(WgcCapturerWindowTest, MinimizeWindowMidCapture) {
  SetUpForWindowCapture();
  EXPECT_TRUE(capturer_->select_source(source_id_));

  capturer_->start(this);

  // Minmize the window and capture should continue but return temporary errors.
  minimize_test_window(window_info_.hwnd);
  for (int i = 0; i < 5; ++i) {
    capturer_->capture_frame();
    EXPECT_EQ(result_, desktop_capturer::capture_result::error_temporary);
  }

  // Reopen the window and the capture should continue normally.
  unminimize_test_window(window_info_.hwnd);
  DoCapture();
  // We can't verify the window size here because the test window does not
  // repaint itself after it is unminimized, but capturing successfully is still
  // a good test.
}

TEST_F(WgcCapturerWindowTest, CloseWindowMidCapture) {
  SetUpForWindowCapture();
  EXPECT_TRUE(capturer_->select_source(source_id_));

  capturer_->start(this);
  DoCapture();
  ValidateFrame(k_medium_window_width, k_medium_window_height);

  close_test_window();

  // We need to pump our message queue so the Closed event will be delivered to
  // the capturer's event handler. If we are too early and the Closed event
  // hasn't arrived yet we should keep trying until the capturer receives it and
  // stops.
  auto *wgc_capturer = static_cast<wgc_capturer_win *>(capturer_.get());
  MSG msg;
  for (int i = 0; wgc_capturer->is_source_being_captured(source_id_) && i < k_max_tries; ++i) {
    // Unlike GetMessage, PeekMessage will not hang if there are no messages in
    // the queue.
    PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
    sleep_ms(1);
  }

  EXPECT_FALSE(wgc_capturer->is_source_being_captured(source_id_));

  // The frame pool can buffer `kNumBuffers` frames. We must consume these
  // and then make one more call to CaptureFrame before we expect to see the
  // failure.
  int num_tries = 0;
  do {
    capturer_->capture_frame();
  } while (result_ == desktop_capturer::capture_result::success &&
           ++num_tries <= wgc_capture_session::k_num_buffers);

  EXPECT_GE(metrics::num_events(k_capturer_result_histogram, k_session_start_failure), 1);
  EXPECT_GE(metrics::num_events(k_capture_session_result_histogram, k_source_closed), 1);
  EXPECT_EQ(result_, desktop_capturer::capture_result::error_permanent);
}

} // namespace base
} // namespace traa

#endif // defined(TRAA_ENABLE_WGC_CAPTURER_TEST)