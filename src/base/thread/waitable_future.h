#ifndef TRAA_BASE_THREAD_WAITABLE_FUTURE_H
#define TRAA_BASE_THREAD_WAITABLE_FUTURE_H

#include <future>
#include <memory>

namespace traa {
namespace base {

enum class waitable_future_status { ready, timeout, deferred, invalid };

template <typename T> class waitable_future {
public:
  waitable_future(std::future<T> &&ft) : shared_ft_(ft.share()) {}

  const T &get(T value) {
    if (shared_ft_.valid()) {
      return shared_ft_.get();
    }

    return value;
  }

  template <class Rep, class Period>
  const T &get_for(const std::chrono::duration<Rep, Period> &timeout, T value) {
    if (shared_ft_.valid()) {
      if (shared_ft_.wait_for(timeout) == std::future_status::ready) {
        return shared_ft_.get();
      }
    }

    return value;
  }

  template <class Clock, class Duration>
  const T &get_until(const std::chrono::time_point<Clock, Duration> &timeout_time, T value) {
    if (shared_ft_.valid()) {
      if (shared_ft_.wait_until(timeout_time) == std::future_status::ready) {
        return shared_ft_.get();
      }
    }

    return value;
  }

  void wait() const {
    if (shared_ft_.valid())
      shared_ft_.wait();
  }

  template <class Rep, class Period>
  waitable_future_status wait_for(const std::chrono::duration<Rep, Period> &timeout) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_for(timeout);
    if (status == std::future_status::ready) {
      return waitable_future_status::ready;
    } else if (status == std::future_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  template <class Clock, class Duration>
  waitable_future_status
  wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_until(timeout_time);
    if (status == std::future_status::ready) {
      return waitable_future_status::ready;
    } else if (status == std::future_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  bool valid() const { return shared_ft_.valid(); }

private:
  std::shared_future<T> shared_ft_;
};

/**
 * A specialization of waitable_future for void futures.
 */
template <> class waitable_future<void> {
public:
  waitable_future(std::future<void> &&ft) : shared_ft_(ft.share()) {}

  void wait() const {
    if (shared_ft_.valid()) {
      shared_ft_.wait();
    }
  }

  template <class Rep, class Period>
  waitable_future_status wait_for(const std::chrono::duration<Rep, Period> &timeout) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_for(timeout);
    if (status == std::future_status::ready) {
      return waitable_future_status::ready;
    } else if (status == std::future_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  template <class Clock, class Duration>
  waitable_future_status
  wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
    if (!shared_ft_.valid()) {
      return waitable_future_status::invalid;
    }

    auto status = shared_ft_.wait_until(timeout_time);
    if (status == std::future_status::ready) {
      return waitable_future_status::ready;
    } else if (status == std::future_status::timeout) {
      return waitable_future_status::timeout;
    } else {
      return waitable_future_status::deferred;
    }
  }

  bool valid() const { return shared_ft_.valid(); }

private:
  std::shared_future<void> shared_ft_;
};
} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_WAITABLE_FUTURE_H_