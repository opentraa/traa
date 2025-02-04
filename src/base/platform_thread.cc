/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/platform_thread.h"

#include <algorithm>
#include <memory>

#if !defined(TRAA_OS_WINDOWS)
#include <sched.h>
#endif

#include "base/checks.h"

namespace traa {
namespace base {

inline namespace {

#if defined(TRAA_OS_WINDOWS)
int win32_priority_from_thread_priority(thread_priority priority) {
  switch (priority) {
  case thread_priority::low:
    return THREAD_PRIORITY_BELOW_NORMAL;
  case thread_priority::normal:
    return THREAD_PRIORITY_NORMAL;
  case thread_priority::high:
    return THREAD_PRIORITY_ABOVE_NORMAL;
  case thread_priority::realtime:
    return THREAD_PRIORITY_TIME_CRITICAL;
  default:
    return THREAD_PRIORITY_NORMAL;
  }
}
#endif

bool set_priority(thread_priority priority) {
#if defined(TRAA_OS_WINDOWS)
  return ::SetThreadPriority(::GetCurrentThread(), win32_priority_from_thread_priority(priority)) !=
         FALSE;
#elif defined(__native_client__) || defined(TRAA_OS_FUCHSIA) ||                                    \
    (defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__))
  // Setting thread priorities is not supported in NaCl, Fuchsia or Emscripten
  // without pthreads.
  return true;
#elif defined(TRAA_OS_CHROMIUM_BUILD) && defined(TRAA_OS_LINUX)
  // TODO(tommi): Switch to the same mechanism as Chromium uses for changing
  // thread priorities.
  return true;
#else
  const int policy = SCHED_FIFO;
  const int min_prio = sched_get_priority_min(policy);
  const int max_prio = sched_get_priority_max(policy);
  if (min_prio == -1 || max_prio == -1) {
    return false;
  }

  if (max_prio - min_prio <= 2)
    return false;

  // Convert webrtc priority to system priorities:
  sched_param param;
  const int top_prio = max_prio - 1;
  const int low_prio = min_prio + 1;
  switch (priority) {
  case thread_priority::low:
    param.sched_priority = low_prio;
    break;
  case thread_priority::normal:
    // The -1 ensures that the kHighPriority is always greater or equal to
    // kNormalPriority.
    param.sched_priority = (low_prio + top_prio - 1) / 2;
    break;
  case thread_priority::high:
    param.sched_priority = std::max(top_prio - 2, low_prio);
    break;
  case thread_priority::realtime:
    param.sched_priority = top_prio;
    break;
  }
  return pthread_setschedparam(pthread_self(), policy, &param) == 0;
#endif // defined(TRAA_OS_WINDOWS)
}

#if defined(TRAA_OS_WINDOWS)
DWORD WINAPI run_platform_thread(void *param) {
  // The GetLastError() function only returns valid results when it is called
  // after a Win32 API function that returns a "failed" result. A crash dump
  // contains the result from GetLastError() and to make sure it does not
  // falsely report a Windows error we call SetLastError here.
  ::SetLastError(ERROR_SUCCESS);
  auto function = static_cast<std::function<void()> *>(param);
  (*function)();
  delete function;
  return 0;
}
#else
void *run_platform_thread(void *param) {
  auto function = static_cast<std::function<void()> *>(param);
  (*function)();
  delete function;
  return 0;
}
#endif // defined(TRAA_OS_WINDOWS)

} // namespace

platform_thread::platform_thread(thread_handle handle, bool joinable)
    : handle_(handle), joinable_(joinable) {}

platform_thread::platform_thread(platform_thread &&rhs)
    : handle_(rhs.handle_), joinable_(rhs.joinable_) {
  rhs.handle_ = std::nullopt;
}

platform_thread &platform_thread::operator=(platform_thread &&rhs) {
  finalize();
  handle_ = rhs.handle_;
  joinable_ = rhs.joinable_;
  rhs.handle_ = std::nullopt;
  return *this;
}

platform_thread::~platform_thread() { finalize(); }

platform_thread platform_thread::spawn_joinable(std::function<void()> thread_function,
                                                std::string_view name,
                                                thread_attributes attributes) {
  return spawn_thread(std::move(thread_function), name, attributes,
                      /*joinable=*/true);
}

platform_thread platform_thread::spawn_detached(std::function<void()> thread_function,
                                                std::string_view name,
                                                thread_attributes attributes) {
  return spawn_thread(std::move(thread_function), name, attributes,
                      /*joinable=*/false);
}

std::optional<platform_thread::thread_handle> platform_thread::get_handle() const {
  return handle_;
}

#if defined(TRAA_OS_WINDOWS)
bool platform_thread::queue_apc(PAPCFUNC function, ULONG_PTR data) {
  TRAA_DCHECK(handle_.has_value());
  return handle_.has_value() ? ::QueueUserAPC(function, *handle_, data) != FALSE : false;
}
#endif

void platform_thread::finalize() {
  if (!handle_.has_value())
    return;
#if defined(TRAA_OS_WINDOWS)
  if (joinable_)
    ::WaitForSingleObject(*handle_, INFINITE);
  ::CloseHandle(*handle_);
#else
  if (joinable_)
    TRAA_CHECK_EQ(0, pthread_join(*handle_, nullptr));
#endif
  handle_ = std::nullopt;
}

platform_thread platform_thread::spawn_thread(std::function<void()> thread_function,
                                              std::string_view name, thread_attributes attributes,
                                              bool joinable) {
  TRAA_DCHECK(thread_function);
  TRAA_DCHECK(!name.empty());
  // TODO(tommi): Consider lowering the limit to 15 (limit on Linux).
  TRAA_DCHECK(name.length() < 64);
  auto start_thread_function_ptr = new std::function<void()>(
      [thread_function = std::move(thread_function), name = std::string(name), attributes] {
        set_current_thread_name(name.c_str());
        set_priority(attributes.priority);
        thread_function();
      });
#if defined(TRAA_OS_WINDOWS)
  // See bug 2902 for background on STACK_SIZE_PARAM_IS_A_RESERVATION.
  // Set the reserved stack stack size to 1M, which is the default on Windows
  // and Linux.
  DWORD thread_id = 0;
  platform_thread::thread_handle handle =
      ::CreateThread(nullptr, 1024 * 1024, &run_platform_thread, start_thread_function_ptr,
                     STACK_SIZE_PARAM_IS_A_RESERVATION, &thread_id);
  TRAA_CHECK(handle) << "CreateThread failed";
#else
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  // Set the stack stack size to 1M.
  pthread_attr_setstacksize(&attr, 1024 * 1024);
  pthread_attr_setdetachstate(&attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
  platform_thread::thread_handle handle;
  TRAA_CHECK_EQ(0, pthread_create(&handle, &attr, &run_platform_thread, start_thread_function_ptr));
  pthread_attr_destroy(&attr);
#endif // defined(TRAA_OS_WINDOWS)
  return platform_thread(handle, joinable);
}

} // namespace base
} // namespace traa
