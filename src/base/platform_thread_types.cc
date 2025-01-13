/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/platform_thread_types.h"

#if defined(TRAA_OS_LINUX)
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif

#if defined(TRAA_OS_WINDOWS)
#include "base/arraysize.h"

// The SetThreadDescription API was brought in version 1607 of Windows 10.
// For compatibility with various versions of winuser and avoid clashing with
// a potentially defined type.
typedef HRESULT(WINAPI *proc_set_thread_description)(HANDLE hThread, PCWSTR lpThreadDescription);
#endif

#if defined(TRAA_OS_FUCHSIA)
#include <string.h>
#include <zircon/syscalls.h>

#include "base/checks.h"
#endif

namespace traa {
namespace base {

platform_thread_id current_thread_id() {
#if defined(TRAA_OS_WINDOWS)
  return GetCurrentThreadId();
#elif defined(TRAA_OS_POSIX)
#if defined(TRAA_OS_MAC) || defined(TRAA_OS_IOS)
  return pthread_mach_thread_np(pthread_self());
#elif defined(TRAA_OS_LINUX_ANDROID)
  return gettid();
#elif defined(TRAA_OS_FUCHSIA)
  return zx_thread_self();
#elif defined(TRAA_OS_LINUX)
  return syscall(__NR_gettid);
#elif defined(__EMSCRIPTEN__)
  return static_cast<platform_thread_id>(pthread_self());
#else
  // Default implementation for nacl and solaris.
  return reinterpret_cast<platform_thread_id>(pthread_self());
#endif
#endif // defined(TRAA_OS_POSIX)
}

platform_thread_ref current_thread_ref() {
#if defined(TRAA_OS_WINDOWS)
  return GetCurrentThreadId();
#elif defined(TRAA_OS_FUCHSIA)
  return zx_thread_self();
#elif defined(TRAA_OS_POSIX)
  return pthread_self();
#endif
}

bool is_thread_ref_equal(const platform_thread_ref &a, const platform_thread_ref &b) {
#if defined(TRAA_OS_WINDOWS) || defined(TRAA_OS_FUCHSIA)
  return a == b;
#elif defined(TRAA_OS_POSIX)
  return pthread_equal(a, b);
#endif
}

void set_current_thread_name(const char *name) {
#if defined(TRAA_OS_WINDOWS)
  // The SetThreadDescription API works even if no debugger is attached.
  // The names set with this API also show up in ETW traces. Very handy.
  static auto set_thread_description_func = reinterpret_cast<proc_set_thread_description>(
      ::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "SetThreadDescription"));
  if (set_thread_description_func) {
    // Convert from ASCII to UTF-16.
    wchar_t wide_thread_name[64];
    for (size_t i = 0; i < arraysize(wide_thread_name) - 1; ++i) {
      wide_thread_name[i] = name[i];
      if (wide_thread_name[i] == L'\0')
        break;
    }
    // Guarantee null-termination.
    wide_thread_name[arraysize(wide_thread_name) - 1] = L'\0';
    set_thread_description_func(::GetCurrentThread(), wide_thread_name);
  }

  // For details see:
  // https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
#pragma pack(push, 8)
  struct {
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
  } threadname_info = {0x1000, name, static_cast<DWORD>(-1), 0};
#pragma pack(pop)

#pragma warning(push)
#pragma warning(disable : 6320 6322)
  __try {
    ::RaiseException(0x406D1388, 0, sizeof(threadname_info) / sizeof(ULONG_PTR),
                     reinterpret_cast<ULONG_PTR *>(&threadname_info));
  } __except (EXCEPTION_EXECUTE_HANDLER) { // NOLINT
  }
#pragma warning(pop)
#elif defined(TRAA_OS_LINUX) || defined(TRAA_OS_LINUX_ANDROID)
  prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name)); // NOLINT
#elif defined(TRAA_OS_MAC) || defined(TRAA_OS_IOS)
  pthread_setname_np(name);
#elif defined(TRAA_OS_FUCHSIA)
  zx_status_t status = zx_object_set_property(zx_thread_self(), ZX_PROP_NAME, name, strlen(name));
  TRAA_DCHECK_EQ(status, ZX_OK);
#endif
}

} // namespace base
} // namespace traa
