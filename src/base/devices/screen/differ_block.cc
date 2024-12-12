/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/differ_block.h"

#include "base/arch.h"
#include "base/system/cpu_features_wrapper.h"

// We use cmake to build for multiple architectures, so we need to check if the current architecture
// is x86_64 family, and if so, enable SSE2 support
#if defined(TRAA_ARCH_X86_FAMILY) && defined(TRAA_ENABLE_SSE2)
#include "base/devices/screen/differ_vector_sse2.h"
#endif // TRAA_ARCH_X86_FAMILY && TRAA_ENABLE_SSE2

#include <string.h>

namespace traa {
namespace base {

inline namespace {

bool vector_difference_c(const uint8_t *image1, const uint8_t *image2) {
  return memcmp(image1, image2, k_differ_block_size * k_differ_bytes_per_pixel) != 0;
}

} // namespace

bool vector_difference(const uint8_t *image1, const uint8_t *image2) {
  static bool (*diff_proc)(const uint8_t *, const uint8_t *) = nullptr;

  if (!diff_proc) {
#if defined(TRAA_ARCH_X86_FAMILY) && defined(TRAA_ENABLE_SSE2)
    bool have_sse2 = get_cpu_info(CPU_FEATURE_X86_SSE2) != 0;
    // For x86 processors, check if SSE2 is supported.
    if (have_sse2 && k_differ_block_size == 32) {
      diff_proc = &vector_difference_sse2_w32;
    } else if (have_sse2 && k_differ_block_size == 16) {
      diff_proc = &vector_difference_sse2_w16;
    } else {
      diff_proc = &vector_difference_c;
    }
#else
    // For other processors, always use C version.
    // TODO(hclam): Implement a NEON version.
    diff_proc = &vector_difference_c;
#endif // TRAA_ARCH_X86_FAMILY && TRAA_ENABLE_SSE2
  }

  return diff_proc(image1, image2);
}

bool block_difference(const uint8_t *image1, const uint8_t *image2, int height, int stride) {
  for (int i = 0; i < height; i++) {
    if (vector_difference(image1, image2)) {
      return true;
    }
    image1 += stride;
    image2 += stride;
  }
  return false;
}

bool block_difference(const uint8_t *image1, const uint8_t *image2, int stride) {
  return block_difference(image1, image2, k_differ_block_size, stride);
}

} // namespace base
} // namespace traa