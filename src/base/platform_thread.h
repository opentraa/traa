/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_PLATFORM_THREAD_H_
#define TRAA_BASE_PLATFORM_THREAD_H_

#include "base/platform.h"

#include <functional>
#include <string>
#if !defined(TRAA_OS_WINDOWS)
#include <pthread.h>
#endif

#include <optional>
#include <string_view>

#include "base/platform_thread_types.h"

namespace traa {
namespace base {

enum class thread_priority {
  low = 1,
  normal,
  high,
  realtime,
};

struct thread_attributes {
  thread_priority priority = thread_priority::normal;
  thread_attributes &set_priority(thread_priority priority_param) {
    priority = priority_param;
    return *this;
  }
};

// Represents a simple worker thread.
class platform_thread final {
public:
  // thread_handle is the base platform thread handle.
#if defined(TRAA_OS_WINDOWS)
  using thread_handle = HANDLE;
#else
  using thread_handle = pthread_t;
#endif // defined(TRAA_OS_WINDOWS)
  // This ctor creates the platform_thread with an unset handle (returning true
  // in empty()) and is provided for convenience.
  // TODO(bugs.webrtc.org/12727) Look into if default and move support can be
  // removed.
  platform_thread() = default;

  // Moves `rhs` into this, storing an empty state in `rhs`.
  // TODO(bugs.webrtc.org/12727) Look into if default and move support can be
  // removed.
  platform_thread(platform_thread &&rhs);

  // Copies won't work since we'd have problems with joinable threads.
  platform_thread(const platform_thread &) = delete;
  platform_thread &operator=(const platform_thread &) = delete;

  // Moves `rhs` into this, storing an empty state in `rhs`.
  // TODO(bugs.webrtc.org/12727) Look into if default and move support can be
  // removed.
  platform_thread &operator=(platform_thread &&rhs);

  // For a platform_thread that's been spawned joinable, the destructor suspends
  // the calling thread until the created thread exits unless the thread has
  // already exited.
  virtual ~platform_thread();

  // Finalizes any allocated resources.
  // For a platform_thread that's been spawned joinable, finalize() suspends
  // the calling thread until the created thread exits unless the thread has
  // already exited.
  // empty() returns true after completion.
  void finalize();

  // Returns true if default constructed, moved from, or finalize()ed.
  bool empty() const { return !handle_.has_value(); }

  // Creates a started joinable thread which will be joined when the returned
  // platform_thread destructs or finalize() is called.
  static platform_thread spawn_joinable(std::function<void()> thread_function,
                                        std::string_view name,
                                        thread_attributes attributes = thread_attributes());

  // Creates a started detached thread. The caller has to use external
  // synchronization as nothing is provided by the platform_thread construct.
  static platform_thread spawn_detached(std::function<void()> thread_function,
                                        std::string_view name,
                                        thread_attributes attributes = thread_attributes());

  // Returns the base platform thread handle of this thread.
  std::optional<thread_handle> get_handle() const;

#if defined(TRAA_OS_WINDOWS)
  // Queue a Windows APC function that runs when the thread is alertable.
  bool queue_apc(PAPCFUNC apc_function, ULONG_PTR data);
#endif

private:
  platform_thread(thread_handle handle, bool joinable);
  static platform_thread spawn_thread(std::function<void()> thread_function, std::string_view name,
                                      thread_attributes attributes, bool joinable);

  std::optional<thread_handle> handle_;
  bool joinable_ = false;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_PLATFORM_THREAD_H_
