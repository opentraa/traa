#ifndef TRAA_BASE_THREAD_TASK_QUEUE_H
#define TRAA_BASE_THREAD_TASK_QUEUE_H

#include "traa/error.h"

#include "base/disallow.h"
#include "base/log/logger.h"
#include "base/singleton.h"
#include "base/thread/rw_lock.h"
#include "base/thread/thread_util.h"

#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/steady_timer.hpp>

namespace traa {
namespace base {

/**
 * @brief A class template for a task timer that repeats execution at a specified interval.
 *
 * This class template provides a mechanism to execute a callable object repeatedly at a specified
 * interval using an `asio::io_context`. The interval is specified in milliseconds.
 *
 * @tparam F The type of the callable object to be executed repeatedly.
 */
template <typename F>
class task_timer_repeatly : public std::enable_shared_from_this<task_timer_repeatly<F>> {
public:
  /**
   * @brief Constructs a task_timer_repeatly object.
   *
   * @param aio The `asio::io_context` object to use for asynchronous operations.
   * @param interval The interval at which the task should be executed, specified in milliseconds.
   * @param f The callable object to be executed repeatedly.
   */
  task_timer_repeatly(asio::io_context &aio, std::chrono::milliseconds interval, F &&f)
      : timer_(aio, interval), interval_(interval), task_(std::forward<F>(f)) {}

  /**
   * @brief Destructor for the task_timer_repeatly class.
   *
   * This destructor is virtual and set to default, meaning it will use the default behavior
   * for destroying objects of this class. It is responsible for cleaning up any resources
   * held by the task_timer_repeatly object.
   */
  virtual ~task_timer_repeatly() = default;

  /**
   * @brief Starts the task timer.
   *
   * This function starts the task timer by scheduling the first execution of the task and setting
   * up subsequent executions at the specified interval.
   */
  void start() {
    auto self = this->shared_from_this();
    timer_.async_wait([self](const asio::error_code &ec) {
      if (!ec) {
        self->timer_.expires_at(self->timer_.expiry() + self->interval_);
        self->task_();
        self->start();
      }
    });
  }

  /**
   * @brief Stops the task timer.
   *
   * This function cancels any pending executions of the task and stops the task timer.
   */
  void stop() { timer_.cancel(); }

private:
  F task_;                             // The callable object to be executed repeatedly.
  std::chrono::milliseconds interval_; // The interval at which the task should be executed.
  asio::steady_timer timer_;           // The steady_timer for task execution.
};

/**
 * @brief Represents a timer that triggers a task once after a specified duration or at a specified
 * time point.
 *
 * @tparam F The type of the task to be executed.
 */
template <typename F>
class task_timer_once : public std::enable_shared_from_this<task_timer_once<F>> {
public:
  /**
   * @brief Constructs a task_timer_once object with a specified duration and task.
   *
   * @param aio The asio::io_context object to be used for asynchronous operations.
   * @param duration The duration after which the task should be executed.
   * @param f The task to be executed.
   */
  task_timer_once(asio::io_context &aio, std::chrono::milliseconds duration, F &&f)
      : timer_(aio, duration), task_(std::forward<F>(f)) {}

  /**
   * @brief Destructor for the task_timer_once class.
   *
   * This destructor is virtual and set to default, meaning it will use the default behavior
   * for destroying objects of this class. It is responsible for cleaning up any resources
   * held by the task_timer_once object.
   */
  virtual ~task_timer_once() = default;

  /**
   * @brief Starts the timer and schedules the task to be executed.
   */
  void start() {
    auto self = this->shared_from_this();
    timer_.async_wait([self](const asio::error_code &ec) {
      if (!ec) {
        self->task_();
      }
    });
  }

  /**
   * @brief Stops the timer and cancels the scheduled task.
   */
  void stop() { timer_.cancel(); }

private:
  F task_;                   // The task to be executed.
  asio::steady_timer timer_; // The timer object used for scheduling the task.
};

/**
 * @class task_queue
 * @brief Represents a task queue for managing asynchronous tasks.
 *
 * The task_queue class provides a mechanism for enqueuing tasks and executing them asynchronously
 * in the io_context's thread pool. It supports various types of task execution, such as executing
 * a task after a specified duration, at a specified time point, or repeatedly at a specified
 * interval.
 */
class task_queue : public std::enable_shared_from_this<task_queue> {
  DISALLOW_COPY_AND_ASSIGN(task_queue);

public:
  using task_queue_id = uint32_t;

private:
  friend class task_queue_manager;

#if defined(TRAA_UNIT_TEST)
public:
#endif

  /**
   * @brief Constructs a task_queue object.
   *
   * This constructor initializes the task_queue object with the specified TLS key, ID, and name.
   *
   * @param tls_key The TLS key for the task queue.
   * @param id The ID of the task queue.
   * @param name The name of the task queue.
   */
  explicit task_queue(std::uintptr_t tls_key, task_queue_id id, const char *name)
      : t_id_(0), tls_key_(tls_key), id_(id), name_(name), work_(asio::make_work_guard(aio_)) {
    t_ = std::thread([this] {
      t_id_ = thread_util::get_thread_id();
      thread_util::set_thread_name(name_.c_str());

      if (tls_key_.load() != UINTPTR_MAX) {
        thread_util::tls_set(tls_key_.load(), this);
      }

      aio_.run();
    });
  }

  /**
   * @brief Creates a new task queue.
   *
   * This function creates a new task queue and returns a shared pointer to it.
   *
   * @param tls_key The TLS key for the task queue.
   * @param id The ID of the task queue.
   * @param name The name of the task queue.
   * @return A shared pointer to the newly created task queue.
   */
  static std::shared_ptr<task_queue> make_queue(std::uintptr_t tls_key, task_queue_id id,
                                                const char *name) {
    return std::shared_ptr<task_queue>(new task_queue(tls_key, id, name));
  }

public:
  /**
   * @brief Destroys the task_queue object.
   *
   * This destructor stops the execution of the io_context by resetting the work guard.
   * It also waits for the thread to finish its execution.
   */
  virtual ~task_queue() { stop(); }

  /**
   * @brief Stops the task_queue.
   *
   * This function stops the execution of the io_context and waits for the thread to finish its
   * execution.
   */
  void stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    aio_.stop();
    if (t_.joinable()) {
      t_.join();
    }
  }

  /**
   * @brief Gets the ID of the task queue.
   *
   * @return The ID of the task queue.
   */
  task_queue_id id() const { return id_.load(); }

  /**
   * @brief Gets the name of the task queue.
   *
   * @return The name of the task queue.
   */
  std::uintptr_t t_id() const { return t_id_.load(); }

  /**
   * @brief Enqueues a task for asynchronous execution.
   *
   * This function takes a callable object and enqueues it for execution in the io_context's thread
   * pool. The task will be executed asynchronously and its result can be obtained through the
   * returned future object.
   *
   * @tparam F The type of the callable object.
   * @param f The callable object to be executed asynchronously.
   * @return A future object representing the result of the task.
   */
  template <typename F> auto enqueue(F &&f) {
    auto task = std::make_shared<std::packaged_task<decltype(f())()>>(std::forward<F>(f));
    asio::post(aio_, [task]() { (*task)(); });
    return task->get_future();
  }

  /**
   * @brief Enqueues a task for asynchronous execution after a specified duration.
   *
   * This function takes a callable object and enqueues it for execution in the io_context's thread
   * pool after a specified duration. The task will be executed asynchronously and its result can be
   * obtained through the returned future object.
   *
   * @tparam F The type of the callable object.
   * @param f The callable object to be executed asynchronously.
   * @param duration The duration after which the task should be executed.
   * @return A future object representing the result of the task.
   */
  template <typename F> auto enqueue_after(F &&f, std::chrono::milliseconds duration) {
    auto timer = std::make_shared<task_timer_once<F>>(aio_, duration, std::forward<F>(f));
    timer->start();
    return timer;
  }

  /**
   * @brief Enqueues a task for asynchronous execution at a specified time point.
   *
   * This function takes a callable object and enqueues it for execution in the io_context's thread
   * pool at a specified time point. The task will be executed asynchronously and its result can be
   * obtained through the returned future object.
   *
   * @tparam F The type of the callable object.
   * @param f The callable object to be executed asynchronously.
   * @param time_point The time point at which the task should be executed.
   * @return A future object representing the result of the task.
   */
  template <typename F>
  auto enqueue_at(F &&f, const std::chrono::system_clock::time_point &time_point) {
    auto timer =
        std::make_shared<task_timer_once<F>>(aio_,
                                             std::chrono::duration_cast<std::chrono::milliseconds>(
                                                 time_point - std::chrono::system_clock::now()),
                                             std::forward<F>(f));
    timer->start();
    return timer;
  }

  /**
   * @brief Enqueues a task for asynchronous execution repeatedly at a specified interval.
   *
   * This function takes a callable object and enqueues it for execution in the io_context's thread
   * pool repeatedly at a specified interval. The task will be executed asynchronously and its
   * result can be obtained through the returned future object.
   *
   * @tparam F The type of the callable object.
   * @param f The callable object to be executed asynchronously.
   * @param interval The interval at which the task should be executed repeatedly.
   * @return A future object representing the result of the task.
   */
  template <typename F> auto enqueue_repeatly(F &&f, std::chrono::milliseconds interval) {
    auto timer = std::make_shared<task_timer_repeatly<F>>(aio_, interval, std::forward<F>(f));
    timer->start();
    return timer;
  }

private:
  std::string name_;                    // The name of the task queue.
  std::thread t_;                       // The thread object that runs the io_context.
  std::mutex mutex_;                    // The mutex to protect the task queue.
  std::atomic<std::uintptr_t> tls_key_; // The TLS key for the task queue.
  std::atomic<task_queue_id> id_;       // The ID of the task queue.
  std::atomic<std::uintptr_t> t_id_;    // The ID of the thread running the task queue.

  asio::io_context aio_; // The io_context for asynchronous task execution.
  asio::executor_work_guard<asio::io_context::executor_type>
      work_; // The work guard to keep the io_context active.
};

/**
 * @class task_queue_manager
 * @brief Manages task queues and provides operations to create_queue, release, and retrieve task
 * queues.
 *
 * The task_queue_manager class is a singleton class that provides a centralized management system
 * for task queues. It allows registering and unregistering task queues, as well as retrieving task
 * queues by their unique identifier. Task queues are stored in an unordered map for efficient
 * lookup.
 *
 * Usage:
 * - Call the init() function to initialize the task queue manager.
 * - Call the shutdown() function to shut down the task queue manager.
 * - Call the get_task_queue_count() function to retrieve the number of task queues.
 * - Call the create_queue() function to create a new task queue.
 * - Call the release_queue() function to release an existing task queue.
 * - Call the get_task_queue() function to retrieve a task queue by its identifier.
 * - Call the post_task() function to post a task to a specific task queue.
 *
 * Example:
 * ```
 * task_queue_manager::init();
 * auto queue = task_queue_manager::create_queue(1, "traa_task_queue");
 * auto res_int queue->enque([]() {
 *     // Task code here
 *     return 1;
 * });
 * int result = res.get();
 *
 * auto res = task_queue_manager::post_task(1, []() {
 *     // Task code here
 * });
 * res.wait();
 * task_queue_manager::shutdown();
 * ```
 */
class task_queue_manager {
  STRICT_SINGLETON_DECLARE(task_queue_manager);

public:
  /**
   * @brief Initializes the task queue.
   * This function allocates a TLS key for the task queue if it has not been allocated already.
   * If the allocation fails, the function logs an error message and aborts the program.
   */
  static void init() {
    LOG_API_NO_ARGS();

    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::WRITE);
    if (self.tls_key_ == UINTPTR_MAX) {
      std::uintptr_t key = UINTPTR_MAX;
      int ret = thread_util::tls_alloc(&key, nullptr);
      if (ret != traa_error::TRAA_ERROR_NONE) {
        LOG_FATAL("failed to alloc tls key for task queue: {}", ret);
        abort();
      } else {
        self.tls_key_ = key;
      }
    }
  }

  /**
   * @brief Shuts down the task queue system.
   *
   * This function stops all task queues and clears the task queue map. It also frees the
   * thread-local storage key. After calling this function, the task queue system will be completely
   * shut down and no further tasks can be enqueued or executed.
   */
  static void shutdown() {
    LOG_API_NO_ARGS();

    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::WRITE);
    for (auto &it : self.task_queues_) {
      it.second->stop();
    }
    self.task_queues_.clear();

    if (self.tls_key_ != UINTPTR_MAX) {
      thread_util::tls_free(self.tls_key_);

      // must reset the TLS key to UINTPTR_MAX, coz task queue manager is a singleton
      self.tls_key_ = UINTPTR_MAX;
    }
  }

  /**
   * Retrieves the thread-local storage key used by the task queue.
   *
   * @return The thread-local storage key.
   */
  static std::uintptr_t get_tls_key() {
    auto &self = instance();
    return self.tls_key_;
  }

  /**
   * @brief Retrieves the number of task queues.
   */
  static size_t get_task_queue_count() {
    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::READ);
    return self.task_queues_.size();
  }

  /**
   * @brief Registers a new task queue.
   * @param id The ID of the task queue to register.
   * @param queue The task queue to register.
   * @return int An error code indicating the result of the operation.
   *
   * This method registers a new task queue with the specified ID. If a task queue with the same ID
   * already exists, an error code is returned. Otherwise, the task queue is registered and an error
   * code indicating success is returned.
   */
  static std::shared_ptr<task_queue> create_queue(task_queue::task_queue_id id, const char *name) {
    LOG_API_TWO_ARGS(id, name);

    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::WRITE);
    if (self.task_queues_.find(id) != self.task_queues_.end()) {
      LOG_ERROR("task queue {} already exists", id);
      return nullptr;
    }

    self.task_queues_[id] = task_queue::make_queue(self.tls_key_, id, name);

    return self.task_queues_[id];
  }

  /**
   * @brief Unregisters a task queue.
   * @param id The ID of the task queue to unregister.
   * @return int An error code indicating the result of the operation.
   *
   * This method unregisters the task queue with the specified ID. If a task queue with the
   * specified ID does not exist, an error code is returned. Otherwise, the task queue is
   * unregistered and an error code indicating success is returned.
   */
  static int release_queue(task_queue::task_queue_id id) {
    LOG_API_ONE_ARG(id);

    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::WRITE);
    auto it = self.task_queues_.find(id);
    if (it == self.task_queues_.end()) {
      LOG_ERROR("task queue {} does not exist", id);
      return traa_error::TRAA_ERROR_NOT_FOUND;
    }

    // TODO @sylar: do we need to call stop?
    // it->second->stop();

    self.task_queues_.erase(it);

    return traa_error::TRAA_ERROR_NONE;
  }

  /**
   * @brief Retrieves a task queue with the given ID.
   * @param id The ID of the task queue to retrieve.
   * @return std::shared_ptr<task_queue> A shared pointer to the task queue, or nullptr if the task
   * queue was not found.
   *
   * This method retrieves the task queue with the specified ID. If a task queue with the specified
   * ID does not exist, nullptr is returned. Otherwise, a shared pointer to the task queue is
   * returned.
   */
  static std::shared_ptr<task_queue> get_task_queue(task_queue::task_queue_id id) {
    // LOG_API_ONE_ARG(id);

    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::READ);
    auto it = self.task_queues_.find(id);
    if (it == self.task_queues_.end()) {
      return nullptr;
    }

    return it->second;
  }

  /**
   * Checks if a task queue with the given ID exists.
   *
   * @param id The ID of the task queue to check.
   * @return `true` if the task queue exists, `false` otherwise.
   */
  static bool is_task_queue_exist(task_queue::task_queue_id id) {
    auto &self = instance();

    rw_lock_guard guard(self.lock_, rw_lock_guard::rw_lock_type::READ);
    return self.task_queues_.find(id) != self.task_queues_.end();
  }

  /**
   * Checks if the current thread is on a task queue.
   *
   * @return true if the current thread is on a task queue, false otherwise.
   */
  static bool is_on_task_queue() {
    auto &self = instance();

    auto queue = static_cast<task_queue *>(thread_util::tls_get(self.tls_key_));
    return queue != nullptr;
  }

  /**
   * @brief Retrieves the task queue for the current thread.
   * @return std::shared_ptr<task_queue> A shared pointer to the task queue for the current thread.
   *
   * This method retrieves the task queue for the current thread. If the current thread is not on a
   * task queue, nullptr is returned. Otherwise, a shared pointer to the task queue is returned.
   */
  static std::shared_ptr<task_queue> get_current_task_queue() {
    auto &self = instance();

    auto queue = static_cast<task_queue *>(thread_util::tls_get(self.tls_key_));
    if (!queue) {
      return nullptr;
    }

    return queue->shared_from_this();
  }

  /**
   * @brief Posts a task to the specified task queue.
   * @param id The ID of the task queue to post the task to.
   * @param f The task to be posted.
   * @return std::future<decltype(f())> A future object representing the result of the task.
   *
   * This method posts a task to the task queue with the specified ID. If a task queue with the
   * specified ID does not exist, nullptr is returned. Otherwise, the task is posted to the task
   * queue and a future object representing the result of the task is returned.
   */
  template <typename F> static auto post_task(task_queue::task_queue_id id, F &&f) {
    // LOG_API_TWO_ARGS(id, reinterpret_cast<std::uintptr_t>(&f));

    auto queue = get_task_queue(id);
    if (!queue) {
      LOG_ERROR("task queue {} does not exist", id);
      return std::future<decltype(f())>();
    }

    return queue->enqueue<F>(std::forward<F>(f));
  }

  /**
   * Posts a task to the current task queue.
   *
   * This function takes a callable object `f` and posts it to the current task queue for execution.
   * If the current thread is not on a task queue, an error message is logged and an empty
   * `std::future` is returned.
   *
   * @tparam F The type of the callable object.
   * @param f The callable object to be executed.
   * @return A `std::future` representing the result of the task.
   */
  template <typename F> static auto post_task(F &&f) {
    auto queue = get_current_task_queue();
    if (!queue) {
      LOG_ERROR("current thread: {} is not on a task queue", thread_util::get_thread_id());
      return std::future<decltype(f())>();
    }

    return queue->enqueue<F>(std::forward<F>(f));
  }

private:
  // The TLS key for the task queues.
  volatile std::uintptr_t tls_key_ = UINTPTR_MAX;

  // The read-write lock for the task queues.
  rw_lock lock_;

  // The task queues.
  std::unordered_map<task_queue::task_queue_id, std::shared_ptr<task_queue>> task_queues_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_TASK_QUEUE_H