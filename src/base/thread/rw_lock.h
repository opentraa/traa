#ifndef TRAA_BASE_THREAD_RW_LOCK_H_
#define TRAA_BASE_THREAD_RW_LOCK_H_

#include "base/disallow.h"

namespace traa {
namespace base {

/**
 * @class rw_lock
 * @brief This class represents a reader-writer lock.
 *
 * The rw_lock class provides a mechanism to control access to a shared resource
 * by multiple readers and writers. Readers can acquire a shared (read) lock,
 * while writers can acquire an exclusive (write) lock. Multiple readers can
 * hold the shared lock simultaneously, but only one writer can hold the
 * exclusive lock at a time.
 *
 * The rw_lock class is non-copyable and non-assignable to prevent unintended
 * usage and potential issues with resource management.
 */
class rw_lock {
  DISALLOW_COPY_AND_ASSIGN(rw_lock);

public:
  /**
   * @brief Constructs a new rw_lock object.
   */
  rw_lock();

  /**
   * @brief Destroys the rw_lock object.
   */
  ~rw_lock();

  /**
   * @brief Acquires a shared (read) lock.
   * @return true if the lock was successfully acquired, false otherwise.
   */
  bool read_lock();

  /**
   * @brief Tries to acquire a shared (read) lock without blocking.
   * @return true if the lock was successfully acquired, false otherwise.
   */
  bool try_read_lock();

  /**
   * @brief Releases a shared (read) lock.
   */
  void read_unlock();

  /**
   * @brief Acquires an exclusive (write) lock.
   * @return true if the lock was successfully acquired, false otherwise.
   */
  bool write_lock();

  /**
   * @brief Tries to acquire an exclusive (write) lock without blocking.
   * @return true if the lock was successfully acquired, false otherwise.
   */
  bool try_write_lock();

  /**
   * @brief Releases an exclusive (write) lock.
   */
  void write_unlock();

private:
  void *rw_lock_; // The underlying reader-writer lock.
};

/**
 * @class rw_lock_guard
 * @brief A class that provides a scoped lock for read-write locks.
 *
 * The `rw_lock_guard` class is used to acquire and release read or write locks
 * on an `rw_lock` object in a scoped manner. When a `rw_lock_guard` object is
 * created, it acquires the lock, and when it goes out of scope, it releases the lock.
 * This ensures that the lock is always released, even in the presence of exceptions.
 *
 * @tparam rw_lock The type of the read-write lock to guard.
 */
class rw_lock_guard {
public:
  /**
   * @brief Constructs a `rw_lock_guard` object and acquires the lock.
   *
   * @param lock The read-write lock to guard.
   * @param write Specifies whether to acquire a write lock (true) or a read lock (false).
   */
  rw_lock_guard(rw_lock &lock, bool is_write) : lock_(lock), is_write_(is_write) {
    if (is_write_) {
      lock_.write_lock();
    } else {
      lock_.read_lock();
    }
  }

  /**
   * @brief Destroys the `rw_lock_guard` object and releases the lock.
   *
   * If the lock was acquired for write, it is released using `write_unlock()`.
   * Otherwise, it is released using `read_unlock()`.
   */
  ~rw_lock_guard() {
    if (is_write_) {
      lock_.write_unlock();
    } else {
      lock_.read_unlock();
    }
  }

private:
  rw_lock &lock_; // The read-write lock being guarded.
  bool is_write_; // Specifies whether the lock was acquired for write or read.
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_RW_LOCK_H_