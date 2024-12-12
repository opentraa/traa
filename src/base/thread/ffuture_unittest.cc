#include <gtest/gtest.h>

#include "base/thread/ffuture.h"

// Test traa::base::fpromise and traa::base::ffuture for basic types
TEST(ffuture_test, basic_type) {
  traa::base::fpromise<int> promise;
  traa::base::ffuture<int> future = promise.get_future();

  EXPECT_TRUE(future.valid());

  promise.set_value(42);

  EXPECT_EQ(future.get(0), 42);
  EXPECT_FALSE(future.valid());
}

// Test traa::base::fpromise and traa::base::ffuture for reference types
TEST(ffuture_test, reference_type) {
  int value = 42;
  traa::base::fpromise<int &> promise;
  traa::base::ffuture<int &> future = promise.get_future();

  EXPECT_TRUE(future.valid());

  promise.set_value(value);

  int default_value = 0;
  EXPECT_EQ(&future.get(default_value), &value);
  EXPECT_FALSE(future.valid());
}

// Test traa::base::fpromise and traa::base::ffuture for void type
TEST(ffuture_test, void_type) {
  traa::base::fpromise<void> promise;
  traa::base::ffuture<void> future = promise.get_future();

  EXPECT_TRUE(future.valid());

  promise.set_value();
  future.get();
  EXPECT_FALSE(future.valid());
}

// Test traa::base::fpromise and traa::base::ffuture for move semantics
TEST(ffuture_test, assigment_promises) {
  // use operator=
  {
    traa::base::fpromise<int> promise;
    traa::base::ffuture<int> future = promise.get_future();
    traa::base::fpromise<int> promise2;
    traa::base::ffuture<int> future2 = promise2.get_future();

    EXPECT_TRUE(future.valid());
    EXPECT_TRUE(future2.valid());

    promise.set_value(42);

    // not allowed to assign promise with an rvalue
    // promise2 = promise;
    promise2 = std::move(promise);

    // after move, future should be still valid and can be get
    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 42);
    EXPECT_FALSE(future.valid());

    // after move future2 should be invalid
    EXPECT_FALSE(future2.valid());

    // expect future2 should be anandoned since the promise is moved
    EXPECT_EQ(future2.wait_for(std::chrono::milliseconds(100)),
              traa::base::ffuture_status::abandoned);

    EXPECT_EQ(future2.get(0), 0);
  }

  // use std::swap
  {
    traa::base::fpromise<int> promise;
    traa::base::ffuture<int> future = promise.get_future();
    traa::base::fpromise<int> promise2;
    traa::base::ffuture<int> future2 = promise2.get_future();

    EXPECT_TRUE(future.valid());
    EXPECT_TRUE(future2.valid());

    promise.set_value(42);
    promise2.set_value(43);

    // not allowed to assign promise with an rvalue
    // promise2 = promise;
    std::swap(promise, promise2);

    // after swap, future should be still valid and can be get
    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 42);
    EXPECT_FALSE(future.valid());

    // after swap future2 should be still valid and can be get
    EXPECT_TRUE(future2.valid());
    EXPECT_EQ(future2.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future2.get(0), 43);
    EXPECT_FALSE(future2.valid());
  }
}

// Test traa::base::fshared_future for basic types
TEST(ffuture_test, shared_basic_type) {
  traa::base::fpromise<int> promise;
  traa::base::ffuture<int> future = promise.get_future();
  traa::base::fshared_future<int> shared_future = future.share();

  EXPECT_TRUE(shared_future.valid());

  promise.set_value(42);

  EXPECT_EQ(shared_future.get(0), 42);
  EXPECT_TRUE(shared_future.valid());
}

// Test traa::base::fshared_future for reference types
TEST(ffuture_test, shared_reference_type) {
  int value = 42;
  traa::base::fpromise<int &> promise;
  traa::base::ffuture<int &> future = promise.get_future();
  traa::base::fshared_future<int &> shared_future = future.share();

  EXPECT_TRUE(shared_future.valid());

  promise.set_value(value);

  int default_value = 0;
  EXPECT_EQ(&shared_future.get(default_value), &value);
  EXPECT_TRUE(shared_future.valid());
}

// Test traa::base::fshared_future for void type
TEST(ffuture_test, shared_void_type) {
  traa::base::fpromise<void> promise;
  traa::base::ffuture<void> future = promise.get_future();
  traa::base::fshared_future<void> shared_future = future.share();

  EXPECT_TRUE(shared_future.valid());

  promise.set_value();
  shared_future.get();
  EXPECT_TRUE(shared_future.valid());
}

// Test wait_for and wait_until
TEST(ffuture_test, wait_for_and_wait_until) {
  {
    traa::base::fpromise<int> promise;
    traa::base::ffuture<int> future = promise.get_future();

    EXPECT_TRUE(future.valid());

    // wait for 100ms
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::timeout);

    promise.set_value(42);

    // wait for 100ms
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 42);
    EXPECT_FALSE(future.valid());
  }

  {
    traa::base::fpromise<int> promise;
    traa::base::ffuture<int> future = promise.get_future();

    EXPECT_TRUE(future.valid());

    // wait until 100ms from now
    auto timeout_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    EXPECT_EQ(future.wait_until(timeout_time), traa::base::ffuture_status::timeout);

    promise.set_value(42);

    // wait until 100ms from now
    EXPECT_EQ(future.wait_until(timeout_time), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 42);
    EXPECT_FALSE(future.valid());
  }
}

// Test fpackaged_task
TEST(ffuture_test, fpackaged_task) {
  traa::base::fpackaged_task<int(int, int)> task([](int a, int b) { return a + b; });
  traa::base::ffuture<int> future = task.get_future();

  EXPECT_TRUE(future.valid());

  task(1, 2);

  EXPECT_EQ(future.get(0), 3);
  EXPECT_FALSE(future.valid());
}

// Test fpackaged_task with void
TEST(ffuture_test, fpackaged_task_void) {
  traa::base::fpackaged_task<void(int &)> task([](int &a) { a = 42; });
  traa::base::ffuture<void> future = task.get_future();

  EXPECT_TRUE(future.valid());

  int value = 0;
  task(value);

  future.get();
  EXPECT_EQ(value, 42);
  EXPECT_FALSE(future.valid());
}

// Test fpackaged_task with move semantics
TEST(ffuture_test, fpackaged_task_move) {
  // use operator=
  {
    traa::base::fpackaged_task<int(int, int)> task([](int a, int b) { return a + b; });
    traa::base::ffuture<int> future = task.get_future();
    traa::base::fpackaged_task<int(int, int)> task2([](int a, int b) { return a + b; });
    traa::base::ffuture<int> future2 = task2.get_future();

    EXPECT_TRUE(future.valid());

    task(1, 2);

    // not allowed to assign task with an rvalue
    // task2 = task;
    task2 = std::move(task);

    // after move, future should be still valid and can be get
    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 3);
    EXPECT_FALSE(future.valid());

    // after move future2 should be invalid
    EXPECT_FALSE(future2.valid());
    EXPECT_EQ(future2.wait_for(std::chrono::milliseconds(100)),
              traa::base::ffuture_status::abandoned);
    EXPECT_EQ(future2.get(0), 0);
  }

  // use std::swap
  {
    traa::base::fpackaged_task<int(int, int)> task([](int a, int b) { return a + b; });
    traa::base::ffuture<int> future = task.get_future();
    traa::base::fpackaged_task<int(int, int)> task2([](int a, int b) { return a + b; });
    traa::base::ffuture<int> future2 = task2.get_future();

    EXPECT_TRUE(future.valid());

    task(1, 2);
    task2(2, 3);

    // not allowed to assign task with an rvalue
    // task2 = task;
    std::swap(task2, task);

    // after swap, future should be still valid and can be get
    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future.get(0), 3);
    EXPECT_FALSE(future.valid());

    // after swap future2 should be  still valid and can be get
    EXPECT_TRUE(future2.valid());
    EXPECT_EQ(future2.wait_for(std::chrono::milliseconds(100)), traa::base::ffuture_status::ready);
    EXPECT_EQ(future2.get(0), 5);
    EXPECT_FALSE(future2.valid());
  }
}