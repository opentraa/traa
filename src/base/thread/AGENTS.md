# Threading Module — Sub-Agent Guide

Parent: [Root AGENTS.md](../../../AGENTS.md)

## Module Identity

This is the threading infrastructure module (`traa::base`), providing ASIO-based task queues, futures, timers, and thread utilities. It is the backbone of traa's concurrency model.

CMake target: `thread` (OBJECT library), alias `traa::base::thread`

## Directory Structure

```
src/base/thread/
├── task_queue.h           # task_queue + task_queue_manager (singleton)
├── ffuture.h/cc           # Custom future/promise (no-exception alternative to std::future)
├── waitable_future.h      # Wrapper adding .get(default_value) and timeout support
├── callback.h             # Weak callback pattern (prevent use-after-free)
├── thread_util.h          # TLS alloc/set/get/free, thread ID, thread naming
├── thread_util_win.cc     # Windows TLS implementation
├── thread_util_posix.cc   # POSIX TLS implementation (macOS/Linux/Android)
├── *_unittest.cc          # Unit tests
```

## Key Classes

### `task_queue` (task_queue.h)
Single-threaded async task executor backed by `asio::io_context`.
- `enqueue(F)` → `waitable_future<R>` — Post a task, get a future
- `enqueue_after(F, duration)` — Delayed one-shot execution
- `enqueue_at(F, time_point)` — Scheduled execution
- `enqueue_repeatly(F, interval)` — Periodic execution
- Each queue runs its own `std::thread` with a dedicated `asio::io_context`
- Uses TLS to detect "am I on this queue's thread?"

### `task_queue_manager` (task_queue.h)
Singleton managing all task queues.
- `init()` / `shutdown()` — Lifecycle (allocates/frees TLS key)
- `create_queue(id, name, exit_callback)` → `shared_ptr<task_queue>`
- `release_queue(id)` — Stop and remove a queue
- `post_task(id, F)` → `waitable_future<R>` — Post to queue by ID
- `post_task(F)` — Post to current thread's queue
- `is_on_task_queue(id)` — Check if current thread is on a specific queue
- Thread-safe via `std::shared_mutex` (read-write lock)

### `ffuture<T>` / `fpromise<T>` (ffuture.h)
Custom future/promise that works without exceptions (unlike `std::future`).
- `fpromise::set_value(T)` — Fulfill the promise
- `ffuture::get()` — Block and get value
- `ffuture::wait_for(duration)` / `wait_until(time_point)` — Timed wait
- `fpackaged_task<R()>` — Packaged task wrapper

### `waitable_future<T>` (waitable_future.h)
Wraps `ffuture<T>` with convenience methods:
- `get(default_value)` — Returns default if future is invalid
- `get_for(duration, default_value)` — Timed get with default
- `wait()` / `wait_for()` / `wait_until()` — Blocking waits

### `weak_callback<T>` / `support_weak_callback` (callback.h)
Pattern to prevent calling callbacks on destroyed objects.
- `support_weak_callback` — Base class, provides `to_weak_callback(closure)`
- `weak_callback<T>` — Wraps a callable, checks validity before invocation
- `make_weak_callback(member_fn, object, args...)` — Factory function
- `weak_callback_flag` — Cancellable callback flag

### `thread_util` (thread_util.h)
Static utility class for thread operations:
- `get_thread_id()` — Current thread ID
- `set_thread_name(name)` — Set thread name (debugging)
- `tls_alloc/set/get/free` — Thread-local storage management

## Timer Classes (in task_queue.h)

### `task_timer_repeatly<F>`
Periodic timer using `asio::steady_timer`. Calls `F` every `interval` milliseconds.
- `start()` — Begin periodic execution
- `stop()` — Cancel timer

### `task_timer_once<F>`
One-shot timer. Calls `F` after `duration` milliseconds.
- `start()` — Schedule execution
- `stop()` — Cancel timer

## Architecture Pattern

The main queue (ID = 0, name "traa_main") serializes all public API calls:

```
User thread → traa_init() → task_queue_manager::post_task(0, lambda)
                                    ↓
                            main queue thread executes lambda
                                    ↓
                            waitable_future.get() returns result to user
```

The engine instance is `thread_local` on the main queue thread, so no locking is needed for engine access within the queue.

## Dependencies

- **ASIO** (header-only): `asio::io_context`, `asio::steady_timer`, `asio::post()`
- Compiled with `ASIO_NO_EXCEPTIONS` and `ASIO_NO_TYPEID`
- Custom exception handler in `task_queue.h` calls `std::terminate()` on ASIO errors

## Adding New Functionality

1. For new async patterns, extend `task_queue` or create new timer types
2. For new thread utilities, add to `thread_util` with platform-specific implementations
3. Always use `waitable_future` for cross-thread result passing
4. Use `weak_callback` when storing callbacks that may outlive the target object

## Testing

- `task_queue_unittest.cc` — Queue creation, enqueue, timers, stop behavior
- `ffuture_unittest.cc` — Future/promise, packaged task
- `waitable_future_unittest.cc` — Timed gets, validity checks
- `callback_unittest.cc` — Weak callback invocation, expiry
- `thread_util_unittest.cc` — TLS operations, thread naming

## Critical Rules

1. Never block the main queue thread — it will deadlock the public API
2. Always use `post_task()` to access the engine, never access it directly from other threads
3. `ffuture`/`waitable_future` are the only safe way to get results across threads (no exceptions)
4. Timer objects must be kept alive (via `shared_ptr`) for the duration of their execution
5. `task_queue::stop()` drains pending tasks and runs the `at_exit` callback
