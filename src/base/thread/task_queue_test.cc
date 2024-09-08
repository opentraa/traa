#include <gtest/gtest.h>

#include "base/thread/task_queue.h"

#include <functional>
#include <memory>
#include <thread>

TEST(task_queue_test, enque) {
  auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");
  EXPECT_TRUE(queue != nullptr);
  EXPECT_TRUE(queue->id() == 1);

  volatile int count = 0;

  for (int i = 0; i < 50; i++) {
    auto future = queue->enqueue([&count]() { return ++count; });
    EXPECT_EQ(future.get(0), count);
  }

  EXPECT_EQ(count, 50);

  // test enqueued task is executed in order
  auto future_1 = queue->enqueue([&count]() { return ++count; });
  auto future_2 = queue->enqueue([&count]() { return ++count; });
  auto future_3 = queue->enqueue([&count]() { return ++count; });
  EXPECT_EQ(future_1.get(0), 51);
  EXPECT_EQ(future_2.get(0), 52);
  EXPECT_EQ(future_3.get(0), 53);
}

TEST(task_queue_test, enqueue_on_queue) {
  auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");

  auto task = std::packaged_task<int()>([]() { return 9527; });
  auto future = task.get_future();

  auto future1 = queue->enqueue([queue, &task]() { queue->enqueue([&task]() { task(); }); });
  future1.wait();

  EXPECT_EQ(future.get(), 9527);
}

TEST(task_queue_test, enque_after) {
  auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");

  // normal case
  {
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto start = std::chrono::system_clock::now();
    auto timer = queue->enqueue_after([&task]() { task(); }, std::chrono::milliseconds(200));
    EXPECT_EQ(future.get(), 9527);
    auto end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);
  }

  // cancel case
  {
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto timer = queue->enqueue_after([&task]() { task(); }, std::chrono::milliseconds(500));
    timer->stop();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(400)), std::future_status::timeout);
  }

  // multiple tasks
  {
    auto task1 = std::packaged_task<int()>([]() { return 9527; });
    auto future1 = task1.get_future();
    auto task2 = std::packaged_task<int()>([]() { return 43; });
    auto future2 = task2.get_future();
    auto start = std::chrono::system_clock::now();
    auto timer1 = queue->enqueue_after([&task1]() { task1(); }, std::chrono::milliseconds(1000));
    auto timer2 = queue->enqueue_after([&task2]() { task2(); }, std::chrono::milliseconds(500));
    EXPECT_EQ(future2.get(), 43);
    auto end2 = std::chrono::system_clock::now();
    EXPECT_EQ(future1.get(), 9527);
    auto end1 = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start).count(), 800);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start).count(), 400);
  }
}

TEST(task_queue_test, enque_at) {
  // normal case
  {
    auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto start = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::now() + std::chrono::milliseconds(400);
    auto timer = queue->enqueue_at([&task]() { task(); }, time_point);
    EXPECT_EQ(future.get(), 9527);
    auto end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 300);
  }

  // cancel case
  {
    auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto time_point = std::chrono::system_clock::now() + std::chrono::milliseconds(200);
    auto timer = queue->enqueue_at([&task]() { task(); }, time_point);
    timer->stop();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(300)), std::future_status::timeout);
  }
}

TEST(task_queue_test, enqueue_repeatly) {
  auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");

  volatile int count = 0;
  auto task = std::packaged_task<int()>([&count]() { return ++count; });
  auto future = task.get_future();
  auto start = std::chrono::system_clock::now();
  auto timer = queue->enqueue_repeatly([&task]() { task(); }, std::chrono::milliseconds(1000));
  EXPECT_EQ(future.get(), 1);
  auto end = std::chrono::system_clock::now();
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 800);

  task.reset();
  auto future2 = task.get_future();

  start = end;
  EXPECT_EQ(future2.get(), 2);
  end = std::chrono::system_clock::now();
  EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 800);

  task.reset();
  auto future3 = task.get_future();

  timer->stop();
  EXPECT_EQ(future3.wait_for(std::chrono::milliseconds(300)), std::future_status::timeout);
}

TEST(task_queue_test, enqueue_at_after_repeatly) {
  auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");

  // normal task
  {
    auto future = queue->enqueue([]() { return 9527; });
    EXPECT_EQ(future.get(0), 9527);
  }

  // after task
  {
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto start = std::chrono::system_clock::now();
    auto timer = queue->enqueue_after([&task]() { task(); }, std::chrono::milliseconds(200));
    EXPECT_EQ(future.get(), 9527);
    auto end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);
  }

  // at task
  {
    auto task = std::packaged_task<int()>([]() { return 9527; });
    auto future = task.get_future();
    auto start = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::now() + std::chrono::milliseconds(200);
    auto timer = queue->enqueue_at([&task]() { task(); }, time_point);
    EXPECT_EQ(future.get(), 9527);
    auto end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);
  }

  // repeatly task
  {
    volatile int count = 0;
    auto task = std::packaged_task<int()>([&count]() { return ++count; });
    auto future = task.get_future();
    auto start = std::chrono::system_clock::now();
    auto timer = queue->enqueue_repeatly([&task]() { task(); }, std::chrono::milliseconds(200));
    EXPECT_EQ(future.get(), 1);
    auto end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    task.reset();
    auto future2 = task.get_future();

    start = end;
    EXPECT_EQ(future2.get(), 2);
    end = std::chrono::system_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);
  }
}

// TODO @sylar: fix the test case
// Still can not find a way to avoid the block of wait after stop and delete the queue before the
// task is executed.
// TEST(task_queue_test, no_block_after_stop_and_delete) {
//   // stop
//   {
//     auto queue = std::make_shared<traa::base::task_queue>(UINTPTR_MAX, 1, "test_queue");
//     queue->stop();
//     auto future = queue->enqueue([]() { return 9527; });
//     future.wait();
//   }
// }

TEST(task_queue_manager_test, init_shutdown) {
  traa::base::task_queue_manager::init();
  EXPECT_NE(traa::base::task_queue_manager::get_tls_key(), UINTPTR_MAX);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 0);
  traa::base::task_queue_manager::shutdown();
  EXPECT_EQ(traa::base::task_queue_manager::get_tls_key(), UINTPTR_MAX);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 0);

  traa::base::task_queue_manager::init();
  EXPECT_NE(traa::base::task_queue_manager::get_tls_key(), UINTPTR_MAX);

  EXPECT_TRUE(traa::base::task_queue_manager::create_queue(1, "test_queue") != nullptr);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 1);

  // create the same queue, expect return nullptr
  EXPECT_TRUE(traa::base::task_queue_manager::create_queue(1, "test_queue") == nullptr);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 1);

  // release the queue
  EXPECT_EQ(traa::base::task_queue_manager::release_queue(1), traa_error::TRAA_ERROR_NONE);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 0);

  // release again, expect return error
  EXPECT_EQ(traa::base::task_queue_manager::release_queue(1), traa_error::TRAA_ERROR_NOT_FOUND);

  // create the same queue again, expect not return nullptr
  EXPECT_TRUE(traa::base::task_queue_manager::create_queue(1, "test_queue") != nullptr);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 1);

  // get the queue
  EXPECT_TRUE(traa::base::task_queue_manager::get_task_queue(1) != nullptr);

  // is the queue exist
  EXPECT_TRUE(traa::base::task_queue_manager::is_task_queue_exist(1));

  // expect is not on the queue
  EXPECT_FALSE(traa::base::task_queue_manager::is_on_task_queue());

  // expect current queue is nullptr
  EXPECT_TRUE(traa::base::task_queue_manager::get_current_task_queue() == nullptr);

  // expect post task with valid queue id return valid future
  EXPECT_TRUE(traa::base::task_queue_manager::post_task(1, []() { return 9527; }).get(0) == 9527);

  // expect post task without queue id return invalid future
  EXPECT_TRUE(traa::base::task_queue_manager::post_task([]() { return 9527; }).valid() == false);

  // expect get current queue return valid queue on the queue
  traa::base::task_queue_manager::post_task(1, []() {
    EXPECT_TRUE(traa::base::task_queue_manager::is_on_task_queue());
    EXPECT_TRUE(traa::base::task_queue_manager::get_current_task_queue() != nullptr);
  }).wait();

  // expect post task without queue id on the queue return valid future
  std::promise<std::uintptr_t> promise;
  auto future = promise.get_future();
  auto task = [&promise]() { promise.set_value(traa::base::thread_util::get_thread_id()); };
  traa::base::task_queue_manager::post_task(1, [&task]() {
    EXPECT_TRUE(traa::base::task_queue_manager::post_task(task).valid());
    // do not wait the future, it will block the current task, coz they are on the
    // same queue
  }).wait();
  EXPECT_TRUE(future.get() == traa::base::task_queue_manager::get_task_queue(1)->t_id());

  traa::base::task_queue_manager::shutdown();
  EXPECT_EQ(traa::base::task_queue_manager::get_tls_key(), UINTPTR_MAX);
  EXPECT_EQ(traa::base::task_queue_manager::get_task_queue_count(), 0);
}