/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/platform_thread.h"

#include "base/system/sleep.h"
#include <gtest/gtest.h>

#include <future>
#include <optional>

namespace traa {
namespace base {

TEST(platform_thread_test, default_constructed_is_empty) {
  platform_thread thread;
  EXPECT_EQ(thread.get_handle(), std::nullopt);
  EXPECT_TRUE(thread.empty());
}

TEST(platform_thread_test, start_finalize) {
  platform_thread thread = platform_thread::spawn_joinable([] {}, "1");
  EXPECT_NE(thread.get_handle(), std::nullopt);
  EXPECT_FALSE(thread.empty());
  thread.finalize();
  EXPECT_TRUE(thread.empty());
  std::promise<void> done;
  thread = platform_thread::spawn_detached([&] { done.set_value(); }, "2");
  EXPECT_FALSE(thread.empty());
  thread.finalize();
  EXPECT_TRUE(thread.empty());
  done.get_future().wait_for(std::chrono::seconds(30));
}

TEST(platform_thread_test, moves_empty) {
  platform_thread thread1;
  platform_thread thread2 = std::move(thread1);
  EXPECT_TRUE(thread1.empty());
  EXPECT_TRUE(thread2.empty());
}

TEST(platform_thread_test, moves_handles) {
  platform_thread thread1 = platform_thread::spawn_joinable([] {}, "1");
  platform_thread thread2 = std::move(thread1);
  EXPECT_TRUE(thread1.empty());
  EXPECT_FALSE(thread2.empty());
  std::promise<void> done;
  thread1 = platform_thread::spawn_detached([&] { done.set_value(); }, "2");
  thread2 = std::move(thread1);
  EXPECT_TRUE(thread1.empty());
  EXPECT_FALSE(thread2.empty());
  done.get_future().wait_for(std::chrono::seconds(30));
}

TEST(platform_thread_test, two_thread_handles_are_different_when_started_and_equal_when_joined) {
  platform_thread thread1 = platform_thread();
  platform_thread thread2 = platform_thread();
  EXPECT_EQ(thread1.get_handle(), thread2.get_handle());
  thread1 = platform_thread::spawn_joinable([] {}, "1");
  thread2 = platform_thread::spawn_joinable([] {}, "2");
  EXPECT_NE(thread1.get_handle(), thread2.get_handle());
  thread1.finalize();
  EXPECT_NE(thread1.get_handle(), thread2.get_handle());
  thread2.finalize();
  EXPECT_EQ(thread1.get_handle(), thread2.get_handle());
}

TEST(platform_thread_test, run_function_is_called) {
  bool flag = false;
  platform_thread::spawn_joinable([&] { flag = true; }, "T");
  EXPECT_TRUE(flag);
}

TEST(platform_thread_test, joins_thread) {
  // This test flakes if there are problems with the join implementation.
  std::promise<void> event;
  platform_thread::spawn_joinable([&] { event.set_value(); }, "T");
  EXPECT_TRUE(event.get_future().wait_for(std::chrono::seconds(0)) == std::future_status::ready);
}

TEST(platform_thread_test, stops_before_detached_thread_exits) {
  // This test flakes if there are problems with the detached thread
  // implementation.
  bool flag = false;
  std::promise<void> thread_started;
  std::promise<void> thread_continue;
  std::promise<void> thread_exiting;
  platform_thread::spawn_detached(
      [&] {
        thread_started.set_value();
        thread_continue.get_future().wait();
        flag = true;
        thread_exiting.set_value();
      },
      "T");
  thread_started.get_future().wait();
  EXPECT_FALSE(flag);
  thread_continue.set_value();
  thread_exiting.get_future().wait();
  EXPECT_TRUE(flag);
}

} // namespace base
} // namespace traa
