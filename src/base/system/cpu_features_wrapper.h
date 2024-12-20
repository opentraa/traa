/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_SYSTEM_CPU_FEATURES_WRAPPER_H_
#define TRAA_BASE_SYSTEM_CPU_FEATURES_WRAPPER_H_

#include <stdint.h>

namespace traa {
namespace base {

// List of features in x86.
typedef enum {
  CPU_FEATURE_X86_SSE2,
  CPU_FEATURE_X86_SSE3,
  CPU_FEATURE_X86_AVX2,
  CPU_FEATURE_X86_FMA3
} CPU_FEATURE_X86;

// List of features in ARM.
typedef enum {
  CPU_FEATURE_ARM_ARMV7 = (1 << 0),
  CPU_FEATURE_ARM_VFPV3 = (1 << 1),
  CPU_FEATURE_ARM_NEON = (1 << 2),
  CPU_FEATURE_ARM_LDREXSTREX = (1 << 3)
} CPU_FEATURE_ARM;

// Returns true if the CPU supports the feature.
int get_cpu_info(CPU_FEATURE_X86 feature);

// No CPU feature is available => straight C path.
int get_cpu_info_no_asm(CPU_FEATURE_X86 feature);

// Return the features in an ARM device.
// It detects the features in the hardware platform, and returns supported
// values in the above enum definition as a bitmask.
uint64_t get_cpu_features_arm(void);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_SYSTEM_CPU_FEATURES_WRAPPER_H_
