/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This header file is used only differ_block.h. It defines the SSE2 rountines
// for finding vector difference.

#ifndef BASE_DEVICES_SCREEN_DIFFER_VECTOR_SSE2_H_
#define BASE_DEVICES_SCREEN_DIFFER_VECTOR_SSE2_H_

#include <stdint.h>

namespace traa {
namespace base {

// Find vector difference of dimension 16.
extern bool vector_difference_sse2_w16(const uint8_t *image1, const uint8_t *image2);

// Find vector difference of dimension 32.
extern bool vector_difference_sse2_w32(const uint8_t *image1, const uint8_t *image2);

} // namespace base
} // namespace traa

#endif // BASE_DEVICES_SCREEN_DIFFER_VECTOR_SSE2_H_
