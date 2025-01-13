/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_PLATFORM_THREAD_TYPES_H_
#define TRAA_BASE_PLATFORM_THREAD_TYPES_H_

#include "base/platform.h"

// clang-format off
// clang formating would change include order.
#if defined(TRAA_OS_WINDOWS)
// Include winsock2.h before including <windows.h> to maintain consistency with
// win32.h. To include win32.h directly, it must be broken out into its own
// build target.
// #include <winsock2.h>
#include <windows.h>
#elif defined(TRAA_OS_FUCHSIA)
#include <zircon/types.h>
#include <zircon/process.h>
#elif defined(TRAA_OS_POSIX)
#include <pthread.h>
#include <unistd.h>
#if defined(TRAA_OS_MAC)
#include <pthread_spis.h>
#endif
#endif
// clang-format on

namespace traa {
namespace base {

#if defined(TRAA_OS_WINDOWS)
typedef DWORD platform_thread_id;
typedef DWORD platform_thread_ref;
#elif defined(TRAA_OS_FUCHSIA)
typedef zx_handle_t platform_thread_id;
typedef zx_handle_t platform_thread_ref;
#elif defined(TRAA_OS_POSIX)
typedef pid_t platform_thread_id;
typedef pthread_t platform_thread_ref;
#endif

// Retrieve the ID of the current thread.
platform_thread_id current_thread_id();

// Retrieves a reference to the current thread. On Windows, this is the same
// as current_thread_id. On other platforms it's the pthread_t returned by
// pthread_self().
platform_thread_ref current_thread_ref();

// Compares two thread identifiers for equality.
bool is_thread_ref_equal(const platform_thread_ref &a, const platform_thread_ref &b);

// Sets the current thread name.
void set_current_thread_name(const char *name);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_PLATFORM_THREAD_TYPES_H_
