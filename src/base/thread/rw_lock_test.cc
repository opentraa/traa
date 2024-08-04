#include <gtest/gtest.h>

#include "base/thread/rw_lock.h"

#include <thread>

TEST(rw_lock_test, read_write_lock) {
  traa::base::rw_lock lock;

  EXPECT_TRUE(lock.read_lock());
  EXPECT_TRUE(lock.read_lock());
  EXPECT_TRUE(lock.try_read_lock());
  EXPECT_FALSE(lock.try_write_lock());

  EXPECT_NO_THROW(lock.read_unlock());
  EXPECT_FALSE(lock.try_write_lock());

  EXPECT_NO_THROW(lock.read_unlock());
  EXPECT_FALSE(lock.try_write_lock());

  EXPECT_NO_THROW(lock.read_unlock());

  EXPECT_TRUE(lock.write_lock());
  EXPECT_FALSE(lock.try_write_lock());
  EXPECT_FALSE(lock.try_read_lock());

  EXPECT_NO_THROW(lock.write_unlock());

  EXPECT_TRUE(lock.try_write_lock());
  EXPECT_FALSE(lock.try_write_lock());
  EXPECT_FALSE(lock.try_read_lock());

  EXPECT_NO_THROW(lock.write_unlock());
}

TEST(rw_lock_test, rw_lock_guard) {
  traa::base::rw_lock lock;

  {
    traa::base::rw_lock_guard guard(lock, traa::base::rw_lock_guard::rw_lock_type::READ);
    EXPECT_FALSE(lock.try_write_lock());
    EXPECT_TRUE(lock.read_lock());
    EXPECT_TRUE(lock.try_read_lock());

    EXPECT_NO_THROW(lock.read_unlock());
    EXPECT_NO_THROW(lock.read_unlock());
    EXPECT_FALSE(lock.try_write_lock());
  }

  {
    traa::base::rw_lock_guard guard(lock, traa::base::rw_lock_guard::rw_lock_type::WRITE);
    EXPECT_FALSE(lock.try_read_lock());
    EXPECT_FALSE(lock.try_write_lock());
  }
}

TEST(rw_lock_test, multi_thread) {
  traa::base::rw_lock lock;

  int i = 0;
  const int max_count = 50;

  std::thread t1([&]() {
    while (true) {
      if (lock.read_lock()) {
        std::cout << "READ Thread 1: " << i << std::endl;
        if (i >= max_count) {
          lock.read_unlock();
          break;
        }

        lock.read_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });

  std::thread t2([&]() {
    while (true) {
      if (lock.try_read_lock()) {
        std::cout << "READ Thread 2: " << i << std::endl;
        if (i >= max_count) {
          lock.read_unlock();
          break;
        }

        lock.read_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  std::thread t3([&]() {
    while (true) {
      if (lock.write_lock()) {
        if (i >= max_count) {
          lock.write_unlock();
          break;
        }

        std::cout << "WRITE Thread 3: " << i++ << std::endl;

        lock.write_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
  });

  std::thread t4([&]() {
    while (true) {
      if (lock.write_lock()) {
        if (i >= max_count) {
          lock.write_unlock();
          break;
        }

        std::cout << "WRITE Thread 4: " << i++ << std::endl;

        lock.write_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  std::thread t5([&]() {
    while (true) {
      if (lock.try_read_lock()) {
        std::cout << "READ Thread 5: " << i << std::endl;
        if (i >= max_count) {
          lock.read_unlock();
          break;
        }

        lock.read_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  });

  std::thread t6([&]() {
    while (true) {
      if (lock.try_write_lock()) {
        if (i >= max_count) {
          lock.write_unlock();
          break;
        }

        std::cout << "WRITE Thread 6: " << i++ << std::endl;

        lock.write_unlock();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  t6.join();

  EXPECT_EQ(i, max_count);
}