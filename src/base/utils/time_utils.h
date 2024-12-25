/*
 *  Copyright 2005 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_TIME_UTILS_H_
#define TRAA_BASE_TIME_UTILS_H_

#include "base/platform.h"

#include <stdint.h>
#include <time.h>

namespace traa {
namespace base {

static constexpr int64_t k_num_millisecs_per_sec = INT64_C(1000);
static constexpr int64_t k_num_microsecs_per_sec = INT64_C(1000000);
static constexpr int64_t k_num_nanosecs_per_sec = INT64_C(1000000000);

static constexpr int64_t k_num_microsecs_per_millisec =
    k_num_microsecs_per_sec / k_num_millisecs_per_sec;
static constexpr int64_t k_num_nanosecs_per_millisec =
    k_num_nanosecs_per_sec / k_num_millisecs_per_sec;
static constexpr int64_t k_num_nanosecs_per_microsec =
    k_num_nanosecs_per_sec / k_num_microsecs_per_sec;

// Elapsed milliseconds between NTP base, 1900 January 1 00:00 GMT
// (see https://tools.ietf.org/html/rfc868), and January 1 00:00 GMT 1970
// epoch. This is useful when converting between the NTP time base and the
// time base used in RTCP reports.
constexpr int64_t k_ntp_jan_1970_millisecs = 2'208'988'800 * k_num_millisecs_per_sec;

// TODO(honghaiz): Define a type for the time value specifically.

class clock_interface {
public:
  virtual ~clock_interface() {}
  virtual int64_t time_nanos() const = 0;
};

// Sets the global source of time. This is useful mainly for unit tests.
//
// Returns the previously set clock_interface, or nullptr if none is set.
//
// Does not transfer ownership of the clock. set_clock_for_testing(nullptr)
// should be called before the clock_interface is deleted.
//
// This method is not thread-safe; it should only be used when no other thread
// is running (for example, at the start/end of a unit test, or start/end of
// main()).
//
// TODO(deadbeef): Instead of having functions that access this global
// clock_interface, we may want to pass the clock_interface into everything
// that uses it, eliminating the need for a global variable and this function.
clock_interface *set_clock_for_testing(clock_interface *clock);

// Returns previously set clock, or nullptr if no custom clock is being used.
clock_interface *get_clock_for_testing();

#if defined(TRAA_OS_WINDOWS_UWP)
// Synchronizes the current clock based upon an NTP server's epoch in
// milliseconds.
void sync_with_ntp(int64_t time_from_ntp_server_ms);

// Returns the current time in nanoseconds. The clock is synchonized with the
// system wall clock time upon instatiation. It may also be synchronized using
// the SyncWithNtp() function above. Please note that the clock will most likely
// drift away from the system wall clock time as time goes by.
int64_t win_uwp_system_time_nanos();
#endif // defined(TRAA_OS_WINDOWS_UWP)

// Returns the actual system time, even if a clock is set for testing.
// Useful for timeouts while using a test clock, or for logging.
int64_t sys_system_time_millis();

// Returns the current time in milliseconds in 32 bits.
uint32_t time_32();

// Returns the current time in milliseconds in 64 bits.
int64_t time_millis();

// Returns the current time in microseconds.
int64_t time_micros();

// Returns the current time in nanoseconds.
int64_t time_nanos();

// Returns a future timestamp, 'elapsed' milliseconds from now.
int64_t time_after(int64_t elapsed);

// Number of milliseconds that would elapse between 'earlier' and 'later'
// timestamps.  The value is negative if 'later' occurs before 'earlier'.
int64_t time_diff(int64_t later, int64_t earlier);
int32_t time_diff_32(uint32_t later, uint32_t earlier);

// The number of milliseconds that have elapsed since 'earlier'.
inline int64_t time_since(int64_t earlier) { return time_millis() - earlier; }

// The number of milliseconds that will elapse between now and 'later'.
inline int64_t time_until(int64_t later) { return later - time_millis(); }

// Convert from tm, which is relative to 1900-01-01 00:00 to number of
// seconds from 1970-01-01 00:00 ("epoch"). Don't return time_t since that
// is still 32 bits on many systems.
int64_t tm_to_seconds(const tm &tm);

// Return the number of microseconds since January 1, 1970, UTC.
// Useful mainly when producing logs to be correlated with other
// devices, and when the devices in question all have properly
// synchronized clocks.
//
// Note that this function obeys the system's idea about what the time
// is. It is not guaranteed to be monotonic; it will jump in case the
// system time is changed, e.g., by some other process calling
// settimeofday. Always use rtc::TimeMicros(), not this function, for
// measuring time intervals and timeouts.
int64_t time_utc_micros();

// Return the number of milliseconds since January 1, 1970, UTC.
// See above.
int64_t time_utc_millis();

} // namespace base
} // namespace traa

#endif // TRAA_BASE_TIME_UTILS_H_
