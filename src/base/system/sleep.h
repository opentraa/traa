/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
// An OS-independent sleep function.

#ifndef TRAA_BASE_SYSTEM_SLEEP_H_
#define TRAA_BASE_SYSTEM_SLEEP_H_

namespace traa {
namespace base {

// TODO @sylar: may be we should use std::this_thread::sleep_for instead of
// Sleep() or nanosleep(). or we should provide a higher precision sleep
// function. like use QueryPerformanceCounter() to sleep on Windows and use
// clock_nanosleep() on Unix.

// This function sleeps for the specified number of milliseconds.
// It may return early if the thread is woken by some other event,
// such as the delivery of a signal on Unix.
void sleep_ms(int msecs);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_SYSTEM_SLEEP_H_
