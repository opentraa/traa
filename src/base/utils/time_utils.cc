/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/utils/time_utils.h"

#include "base/system/system_time.h"

#include <stdint.h>

#if defined(TRAA_OS_POSIX)
#include <sys/time.h>
#endif

#if defined(TRAA_OS_WINDOWS)
#include "base/win32.h"
// must be after win32.h
#include <minwinbase.h>
#endif

#if defined(TRAA_OS_WINDOWS) || defined(TRAA_OS_WINDOWS_UWP)
// FileTime (January 1st 1601) to Unix time (January 1st 1970)
// offset in units of 100ns.
static constexpr uint64_t k_file_time_to_unix_time_epoch_offset = 116444736000000000ULL;
static constexpr uint64_t k_file_time_to_micro_seconds = 10LL;
#endif

namespace traa {
namespace base {

inline namespace {

clock_interface *g_clock = nullptr;

#if defined(TRAA_OS_WINDOWS_UWP)

class time_helper final {
public:
  time_helper(const time_helper &) = delete;

  // Resets the clock based upon an NTP server. This routine must be called
  // prior to the main system start-up to ensure all clocks are based upon
  // an NTP server time if NTP synchronization is required. No critical
  // section is used thus this method must be called prior to any clock
  // routines being used.
  static void sync_with_ntp(int64_t ntp_server_time_ms) {
    auto &ins = instance();
    TIME_ZONE_INFORMATION time_zone;
    ::GetTimeZoneInformation(&time_zone);
    int64_t time_zone_bias_ns = static_cast<int64_t>(time_zone.Bias) * 60 * 1000 * 1000 * 1000;
    ins.app_start_time_ns_ =
        (ntp_server_time_ms - k_ntp_time_to_unix_time_epoch_offset) * 1000000 - time_zone_bias_ns;
    ins.update_reference_time();
  }

  // Returns the number of nanoseconds that have passed since unix epoch.
  static int64_t ticks_ns() {
    auto &ins = instance();
    int64_t result = 0;
    LARGE_INTEGER qpcnt;
    ::QueryPerformanceCounter(&qpcnt);
    result = static_cast<int64_t>((static_cast<uint64_t>(qpcnt.QuadPart) * 100000 /
                                   static_cast<uint64_t>(ins.os_ticks_per_second_)) *
                                  10000);
    result = ins.app_start_time_ns_ + result - ins.time_since_os_start_ns_;
    return result;
  }

private:
  time_helper() {
    TIME_ZONE_INFORMATION time_zone;
    ::GetTimeZoneInformation(&time_zone);
    int64_t time_zone_bias_ns = static_cast<int64_t>(time_zone.Bias) * 60 * 1000 * 1000 * 1000;
    FILETIME ft;
    // This will give us system file in UTC format.
    ::GetSystemTimeAsFileTime(&ft);
    LARGE_INTEGER li;
    li.HighPart = ft.dwHighDateTime;
    li.LowPart = ft.dwLowDateTime;

    app_start_time_ns_ =
        (li.QuadPart - k_file_time_to_unix_time_epoch_offset) * 100 - time_zone_bias_ns;

    update_reference_time();
  }

  static time_helper &instance() {
    static time_helper ins;
    return ins;
  }

  void update_reference_time() {
    LARGE_INTEGER qpfreq;
    ::QueryPerformanceFrequency(&qpfreq);
    os_ticks_per_second_ = static_cast<int64_t>(qpfreq.QuadPart);

    LARGE_INTEGER qpcnt;
    ::QueryPerformanceCounter(&qpcnt);
    time_since_os_start_ns_ = static_cast<int64_t>((static_cast<uint64_t>(qpcnt.QuadPart) * 100000 /
                                                    static_cast<uint64_t>(os_ticks_per_second_)) *
                                                   10000);
  }

private:
  static constexpr uint64_t k_ntp_time_to_unix_time_epoch_offset = 2208988800000L;

  // The number of nanoseconds since unix system epoch
  int64_t app_start_time_ns_;
  // The number of nanoseconds since the OS started
  int64_t time_since_os_start_ns_;
  // The OS calculated ticks per second
  int64_t os_ticks_per_second_;
};

void sync_with_ntp(int64_t time_from_ntp_server_ms) {
  time_helper::sync_with_ntp(time_from_ntp_server_ms);
}

int64_t win_uwp_system_time_nanos() { return time_helper::ticks_ns(); }

#endif // defined(TRAA_OS_WINDOWS_UWP)

} // namespace

clock_interface *set_clock_for_testing(clock_interface *clock) {
  clock_interface *prev = g_clock;
  g_clock = clock;
  return prev;
}

clock_interface *get_clock_for_testing() { return g_clock; }

int64_t sys_system_time_millis() {
  return static_cast<int64_t>(system_time_nanos() / k_num_nanosecs_per_millisec);
}

int64_t time_nanos() {
  if (g_clock) {
    return g_clock->time_nanos();
  }
  return system_time_nanos();
}

uint32_t Time32() { return static_cast<uint32_t>(time_nanos() / k_num_nanosecs_per_millisec); }

int64_t time_millis() { return time_nanos() / k_num_nanosecs_per_millisec; }

int64_t time_micros() { return time_nanos() / k_num_nanosecs_per_microsec; }

int64_t time_after(int64_t elapsed) { return time_millis() + elapsed; }

int32_t time_diff_32(uint32_t later, uint32_t earlier) { return later - earlier; }

int64_t time_diff(int64_t later, int64_t earlier) { return later - earlier; }

int64_t tm_to_seconds(const tm &tm) {
  static short int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  static short int cumul_mdays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int year = tm.tm_year + 1900;
  int month = tm.tm_mon;
  int day = tm.tm_mday - 1; // Make 0-based like the rest.
  int hour = tm.tm_hour;
  int min = tm.tm_min;
  int sec = tm.tm_sec;

  bool expiry_in_leap_year = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

  if (year < 1970)
    return -1;
  if (month < 0 || month > 11)
    return -1;
  if (day < 0 || day >= mdays[month] + (expiry_in_leap_year && month == 2 - 1))
    return -1;
  if (hour < 0 || hour > 23)
    return -1;
  if (min < 0 || min > 59)
    return -1;
  if (sec < 0 || sec > 59)
    return -1;

  day += cumul_mdays[month];

  // Add number of leap days between 1970 and the expiration year, inclusive.
  day += ((year / 4 - 1970 / 4) - (year / 100 - 1970 / 100) + (year / 400 - 1970 / 400));

  // We will have added one day too much above if expiration is during a leap
  // year, and expiration is in January or February.
  if (expiry_in_leap_year && month <= 2 - 1) // `month` is zero based.
    day -= 1;

  // Combine all variables into seconds from 1970-01-01 00:00 (except `month`
  // which was accumulated into `day` above).
  return (((static_cast<int64_t>(year - 1970) * 365 + day) * 24 + hour) * 60 + min) * 60 + sec;
}

int64_t time_utc_micros() {
  if (g_clock) {
    return g_clock->time_nanos() / k_num_nanosecs_per_microsec;
  }
#if defined(TRAA_OS_POSIX)
  struct timeval time;
  gettimeofday(&time, nullptr);
  // Convert from second (1.0) and microsecond (1e-6).
  return (static_cast<int64_t>(time.tv_sec) * k_num_microsecs_per_sec + time.tv_usec);
#elif defined(TRAA_OS_WINDOWS)
  FILETIME ft;
  // This will give us system file in UTC format in multiples of 100ns.
  ::GetSystemTimeAsFileTime(&ft);
  LARGE_INTEGER li;
  li.HighPart = ft.dwHighDateTime;
  li.LowPart = ft.dwLowDateTime;
  return (li.QuadPart - k_file_time_to_unix_time_epoch_offset) / k_file_time_to_micro_seconds;
#endif
}

int64_t time_utc_millis() { return time_utc_micros() / k_num_microsecs_per_millisec; }

} // namespace base
} // namespace traa
