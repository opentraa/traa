/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/screen_drawer.h"

#include "base/checks.h"
#include "base/function_view.h"
#include "base/logger.h"
#include "base/platform.h"
#include "base/random.h"
#include "base/system/sleep.h"
#include "base/utils/time_utils.h"

#if defined(TRAA_OS_POSIX)
#include "base/devices/screen/test/screen_drawer_lock_posix.h"
#endif

#include <gtest/gtest.h>

#include <stdint.h>

#include <atomic>
#include <memory>
#include <thread>

namespace traa {
namespace base {

namespace {

void test_screen_drawer_lock(function_view<std::unique_ptr<screen_drawer_lock>()> ctor) {
  constexpr int k_lock_duration_ms = 100;

  std::atomic<bool> created(false);
  std::atomic<bool> ready(false);

  class task_t {
  public:
    task_t(std::atomic<bool> *created, const std::atomic<bool> &ready,
           function_view<std::unique_ptr<screen_drawer_lock>()> ctor)
        : created_(created), ready_(ready), ctor_(ctor) {}

    ~task_t() = default;

    void run_task() {
      std::unique_ptr<screen_drawer_lock> lock = ctor_();
      ASSERT_TRUE(!!lock);
      created_->store(true);
      // Wait for the main thread to get the signal of created_.
      while (!ready_.load()) {
        sleep_ms(1);
      }
      // At this point, main thread should begin to create a second lock. Though
      // it's still possible the second lock won't be created before the
      // following sleep has been finished, the possibility will be
      // significantly reduced.
      const int64_t current_ms = time_millis();
      // sleep_ms() may return early. See
      // https://cs.chromium.org/chromium/src/third_party/webrtc/system_wrappers/include/sleep.h?rcl=4a604c80cecce18aff6fc5e16296d04675312d83&l=20
      // But we need to ensure at least 100 ms has been passed before unlocking
      // `lock`.
      while (time_millis() - current_ms < k_lock_duration_ms) {
        sleep_ms(k_lock_duration_ms - static_cast<int>(time_millis() - current_ms));
      }
    }

  private:
    std::atomic<bool> *const created_;
    const std::atomic<bool> &ready_;
    const function_view<std::unique_ptr<screen_drawer_lock>()> ctor_;
  } task(&created, ready, ctor);

  auto lock_thread = std::thread([&task] { task.run_task(); });

  // Wait for the first lock in task_t::run_task() to be created.
  // TODO(zijiehe): Find a better solution to wait for the creation of the first
  // lock. See
  // https://chromium-review.googlesource.com/c/607688/13/webrtc/modules/desktop_capture/screen_drawer_unittest.cc
  while (!created.load()) {
    sleep_ms(1);
  }

  const int64_t start_ms = time_millis();
  ready.store(true);
  // This is unlikely to fail, but just in case current thread is too laggy and
  // cause the sleep_ms() in RunTask() to finish before we creating another lock.
  ASSERT_GT(k_lock_duration_ms, time_millis() - start_ms);
  ctor();
  ASSERT_LE(k_lock_duration_ms, time_millis() - start_ms);

  if (lock_thread.joinable()) {
    lock_thread.join();
  }
}

} // namespace

// These are a set of manual test cases, as we do not have an automatical way to
// detect whether a screen_drawer on a certain platform works well without
// screen_capturer(s). So you may execute these test cases with
// --gtest_also_run_disabled_tests --gtest_filter=screen_drawer_test.*.
TEST(screen_drawer_test, DISABLED_draw_rectangles) {
  std::unique_ptr<screen_drawer> drawer = screen_drawer::create();
  if (!drawer) {
    LOG_WARN("no screen_drawer implementation for current platform.");
    return;
  }

  if (drawer->drawable_region().is_empty()) {
    LOG_WARN("screen_drawer of current platform does not provide a "
             "non-empty drawable_region().");
    return;
  }

  desktop_rect rect = drawer->drawable_region();
  traa::base::random rd(time_micros());
  for (int i = 0; i < 100; i++) {
    // Make sure we at least draw one pixel.
    int left = rd.rand(rect.left(), rect.right() - 2);
    int top = rd.rand(rect.top(), rect.bottom() - 2);
    drawer->draw_rectangle(
        desktop_rect::make_ltrb(left, top, rd.rand(left + 1, rect.right()),
                                rd.rand(top + 1, rect.bottom())),
        rgba_color(rd.rand<uint8_t>(), rd.rand<uint8_t>(), rd.rand<uint8_t>(), rd.rand<uint8_t>()));

    if (i == 50) {
      sleep_ms(10000);
    }
  }

  sleep_ms(10000);
}

#if defined(THREAD_SANITIZER) // bugs.webrtc.org/10019
#define MAYBE_two_screen_drawer_locks DISABLED_two_screen_drawer_locks
#else
#define MAYBE_two_screen_drawer_locks two_screen_drawer_locks
#endif
TEST(screen_drawer_test, MAYBE_two_screen_drawer_locks) {
#if defined(TRAA_OS_POSIX)
  // ScreenDrawerLockPosix won't be able to unlink the named semaphore. So use a
  // different semaphore name here to avoid deadlock.
  const char *semaphore_name = "GSDL8784541a812011e788ff67427b";
  screen_drawer_lock_posix::unlink(semaphore_name);

  test_screen_drawer_lock(
      [semaphore_name]() { return std::make_unique<screen_drawer_lock_posix>(semaphore_name); });
#elif defined(TRAA_OS_WINDOWS)
  test_screen_drawer_lock([]() { return screen_drawer_lock::create(); });
#endif
}

} // namespace base
} // namespace traa
