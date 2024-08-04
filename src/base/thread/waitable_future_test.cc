#include <gtest/gtest.h>

#include "base/thread/waitable_future.h"

#include <thread>

using namespace traa::base;

TEST(waitable_future_test, get) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_EQ(waitable_res.get(9527), 9527);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    EXPECT_EQ(waitable_res.get(0), 9527);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 900);

    t.join();
  }
}

TEST(waitable_future_test, get_for) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(500), 9527), 9527);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 400);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(200), 0), 0);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(1000), 0), 9527);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_test, get_until) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(100);
    EXPECT_EQ(waitable_res.get_until(timeout_time, 9527), 9527);
    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.get_until(timeout_time, 0), 0);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(800);
    EXPECT_EQ(waitable_res.get_until(timeout_time, 0), 9527);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_test, wait) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 900);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, wait_for) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(100)),
              waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(200)),
              waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(800)), waitable_future_status::ready);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, wait_until) {
  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(100);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(800);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::ready);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, valid) {
  // valid future
  {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_TRUE(waitable_res.valid());

    promise.set_value(9527);

    EXPECT_TRUE(waitable_res.valid());
  }

  // invalid future
  {
    std::future<int> future;
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_FALSE(waitable_res.valid());
  }
}

TEST(waitable_future_void_test, wait) {
  // invalid future
  {
    std::future<void> future;
    waitable_future<void> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value();
    });

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 900);

    t.join();
  }
}

TEST(waitable_future_void_test, wait_for) {
  // invalid future
  {
    std::future<void> future;
    waitable_future<void> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(100)),
              waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value();
    });

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(200)),
              waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(800)), waitable_future_status::ready);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_void_test, wait_until) {
  // invalid future
  {
    std::future<void> future;
    waitable_future<void> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(100);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value();
    });

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(800);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::ready);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_void_test, valid) {
  // valid future
  {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    EXPECT_TRUE(waitable_res.valid());

    promise.set_value();

    EXPECT_TRUE(waitable_res.valid());
  }

  // invalid future
  {
    std::future<void> future;
    waitable_future<void> waitable_res(std::move(future));

    EXPECT_FALSE(waitable_res.valid());
  }
}