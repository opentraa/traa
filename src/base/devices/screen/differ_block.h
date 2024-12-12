/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef BASE_DEVICES_SCREEN_DIFFER_BLOCK_H_
#define BASE_DEVICES_SCREEN_DIFFER_BLOCK_H_

#include <stdint.h>

namespace traa {
namespace base {

// Size (in pixels) of each square block used for diffing. This must be a
// multiple of sizeof(uint64)/8.
constexpr int k_differ_block_size = 32;

// Format: BGRA 32 bit.
constexpr int k_differ_bytes_per_pixel = 4;

// Low level function to compare 2 vectors of pixels of size k_differ_block_size. Returns
// whether the blocks differ.
bool vector_difference(const uint8_t *image1, const uint8_t *image2);

// Low level function to compare 2 blocks of pixels of size
// (k_differ_block_size, `height`).  Returns whether the blocks differ.
bool block_difference(const uint8_t *image1, const uint8_t *image2, int height, int stride);

// Low level function to compare 2 blocks of pixels of size
// (k_differ_block_size, k_differ_block_size).  Returns whether the blocks differ.
bool block_difference(const uint8_t *image1, const uint8_t *image2, int stride);

} // namespace base
} // namespace traa

#endif // BASE_DEVICES_SCREEN_DIFFER_BLOCK_H_
