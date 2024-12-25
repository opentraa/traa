/*
 *  Copyright 2021 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_SYSTEM_TIME_H_
#define TRAA_BASE_SYSTEM_TIME_H_

#include <cstdint>

namespace traa {
namespace base {

// Returns the actual system time, even if a clock is set for testing.
// Useful for timeouts while using a test clock, or for logging.
int64_t system_time_nanos();

} // namespace base
} // namespace traa
#endif // TRAA_BASE_SYSTEM_TIME_H_
