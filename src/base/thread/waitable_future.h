#ifndef TRAA_BASE_THREAD_WAITABLE_FUTURE_H_
#define TRAA_BASE_THREAD_WAITABLE_FUTURE_H_

#include <chrono>
#include <memory>

#include "base/thread/ffuture.h"

namespace traa {
namespace base {

/**
 * The status of a waitable_future.
 *
 * ready: The future is ready.
 * timeout: The future is not ready and the timeout has expired.
 * deferred: The future is not ready and the timeout has not expired.
 * invalid: The future is invalid.
 */
enum class waitable_future_status { ready, timeout, deferred, invalid };

/**
 * @brief A class that wraps a ffuture and provides additional functionality for waiting and
 * retrieving the result.
 *
 * @tparam T The type of the value stored in the ffuture.
 */
template <typename T> class waitable_future {
public:
  waitable_future() {}
  /**
   * @brief Constructs a waitable_future object by moving a ffuture.
   *
   * @param ft The ffuture to be moved.
   */
  waitable_future(ffuture<T> &&ft) : shared_ft_(ft.share()) {}

  /**
   * @brief Waits for the result of the ffuture and returns it, or returns a default value if
   * the ffuture is not valid.
   *
   * @param value The default value to be returned if the ffuture is not valid.
   * @return T The result of the ffuture or the default value.
   */
  T get(T value) {
    if (shared_ft_.valid()) {
      return shared_ft_.get(value);
    }

    return value;
  }

  /**
   * @brief Waits for the result of the ffuture for a specified duration and returns it, or
   * returns a default value if the ffuture is not valid or the timeout is reached.
   *
   * @tparam Rep The type representing the number of ticks in the duration.
   * @tparam Period The type representing the tick period of the duration.
   * @param timeout The maximum duration to wait for the result.
   * @param value The default value to be returned if the ffuture is not valid or the timeout is
   * reached.
   * @return T The result of the ffuture or the default value.
   */
  template <class Rep, class Period>
  T get_for(const std::chrono::duration<Rep, Period> &timeout, T value) {
    if (shared_ft_.valid()) {
      if (shared_ft_.wait_for(timeout) == ffuture_status::ready) {
        return shared_ft_.get(value);
      }
    }

    return value;
  }

  /**
   * @brief Waits for the result of the ffuture until a specified time point and returns it, or
   * returns a default value if the ffuture is not valid or the timeout is reached.
   *
   * @tparam Clock The clock type used to measure time.
   * @tparam Duration The duration type used to represent the time interval.
   * @param timeout_time The time point until which to wait for the result.
   * @param value The default value to be returned if the ffuture is not valid or the timeout is
   * reached.
   * @return T The result of the ffuture or the default value.
   */
  template <class Clock, class Duration>
  T get_until(const std::chrono::time_point<Clock, Duration> &timeout_time, T value) {
    if (shared_ft_.valid()) {
      if (shared_ft_.wait_until(timeout_time) == ffuture_status::ready) {
        return shared_ft_.get(value);
      }
    }

    return value;
  }

  /**
   * @brief Waits for the result of the ffuture.
   */
  void wait() const {
    if (shared_ft_.valid())
      shared_ft_.wait();
  }

  /**
   * @brief Waits for the result of the ffuture for a specified duration.
   *
   * @tparam Rep The type representing the number of ticks in the duration.
   * @tparam Period The type representing the tick period of the duration.
   * @param timeout The maximum duration to wait for the result.
   * @return waitable_future_status The status of the wait operation.
   */
  template <class Rep, class Period>
  waitable_future_status wait_for(const std::chrono::duration<Rep, Period> &timeout) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_for(timeout);
    if (status == ffuture_status::ready) {
      return waitable_future_status::ready;
    } else if (status == ffuture_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  /**
   * @brief Waits for the result of the ffuture until a specified time point.
   *
   * @tparam Clock The clock type used to measure time.
   * @tparam Duration The duration type used to represent the time interval.
   * @param timeout_time The time point until which to wait for the result.
   * @return waitable_future_status The status of the wait operation.
   */
  template <class Clock, class Duration>
  waitable_future_status
  wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_until(timeout_time);
    if (status == ffuture_status::ready) {
      return waitable_future_status::ready;
    } else if (status == ffuture_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  /**
   * @brief Checks if the ffuture is valid.
   *
   * @return bool True if the ffuture is valid, false otherwise.
   */
  bool valid() const { return shared_ft_.valid(); }

private:
  // The shared future object.
  fshared_future<T> shared_ft_;
};

/**
 * @brief A class template that represents a waitable future for void.
 *
 * This class provides a way to wait for the completion of a void future and check its status.
 */
template <> class waitable_future<void> {
public:
  waitable_future() {}
  /**
   * @brief Constructs a waitable_future object from a ffuture<void> object.
   *
   * @param ft The ffuture<void> object to be shared.
   */
  waitable_future(ffuture<void> &&ft) : shared_ft_(ft.share()) {}

  /**
   * @brief Waits until the associated future becomes ready.
   *
   * If the associated future is valid, this function blocks until the future becomes ready.
   */
  void wait() const {
    if (shared_ft_.valid()) {
      shared_ft_.wait();
    }
  }

  /**
   * @brief Waits for the associated future to become ready, with a timeout duration.
   *
   * If the associated future is not valid, this function returns waitable_future_status::invalid.
   * If the associated future becomes ready within the specified timeout duration, this function
   * returns waitable_future_status::ready. If the timeout duration is reached before the associated
   * future becomes ready, this function returns waitable_future_status::timeout.
   *
   * @tparam Rep The type of the timeout duration's count.
   * @tparam Period The type of the timeout duration's period.
   * @param timeout The timeout duration.
   * @return The status of the associated future.
   */
  template <class Rep, class Period>
  waitable_future_status wait_for(const std::chrono::duration<Rep, Period> &timeout) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_for(timeout);
    if (status == ffuture_status::ready) {
      return waitable_future_status::ready;
    } else if (status == ffuture_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  /**
   * @brief Waits until the associated future becomes ready, until a specific time point.
   *
   * If the associated future is not valid, this function returns waitable_future_status::invalid.
   * If the associated future becomes ready before the specified time point, this function returns
   * waitable_future_status::ready. If the specified time point is reached before the associated
   * future becomes ready, this function returns waitable_future_status::timeout.
   *
   * @tparam Clock The type of the clock used to measure time.
   * @tparam Duration The type of the duration used to represent time intervals.
   * @param timeout_time The time point until which to wait.
   * @return The status of the associated future.
   */
  template <class Clock, class Duration>
  waitable_future_status
  wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_until(timeout_time);
    if (status == ffuture_status::ready) {
      return waitable_future_status::ready;
    } else if (status == ffuture_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  /**
   * @brief Checks if the associated future is valid.
   *
   * @return True if the associated future is valid, false otherwise.
   */
  bool valid() const { return shared_ft_.valid(); }

private:
  // The shared future object.
  fshared_future<void> shared_ft_;
};
} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_WAITABLE_FUTURE_H_