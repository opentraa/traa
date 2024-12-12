/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/system/cpu_info.h"

#include "base/log/logger.h"
#include "base/platform.h"

#if defined(TRAA_OS_WINDOWS)
#include <windows.h>
#elif defined(TRAA_OS_LINUX)
#include <unistd.h>
#elif defined(TRAA_OS_MAC)
#include <sys/sysctl.h>
#elif defined(TRAA_OS_FUCHSIA)
#include <zircon/syscalls.h>
#endif

namespace traa {
namespace base {

inline namespace {
static int internal_detect_number_of_cores() {
  int number_of_cores;

#if defined(TRAA_OS_WINDOWS)
  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);
  number_of_cores = static_cast<int>(si.dwNumberOfProcessors);
#elif defined(TRAA_OS_LINUX) || defined(TRAA_OS_LINUX_ANDROID)
  number_of_cores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
  if (number_of_cores <= 0) {
    LOG_ERROR("Failed to get number of cores");
    number_of_cores = 1;
  }
#elif defined(TRAA_OS_MAC) || defined(TRAA_OS_IOS)
  int name[] = {CTL_HW, HW_AVAILCPU};
  size_t size = sizeof(number_of_cores);
  if (0 != sysctl(name, 2, &number_of_cores, &size, NULL, 0)) {
    LOG_ERROR("Failed to get number of cores");
    number_of_cores = 1;
  }
#elif defined(TRAA_OS_FUCHSIA)
  number_of_cores = zx_system_get_num_cpus();
#else
  LOG_ERROR("No function to get number of cores");
  number_of_cores = 1;
#endif

  LOG_INFO("Available number of cores: {}", number_of_cores);

  return number_of_cores;
}
} // namespace

uint32_t cpu_info::detect_number_of_cores() {
  // Statically cache the number of system cores available since if the process
  // is running in a sandbox, we may only be able to read the value once (before
  // the sandbox is initialized) and not thereafter.
  // For more information see crbug.com/176522.
  static const uint32_t logical_cpus = static_cast<uint32_t>(internal_detect_number_of_cores());
  return logical_cpus;
}

} // namespace base
} // namespace traa
