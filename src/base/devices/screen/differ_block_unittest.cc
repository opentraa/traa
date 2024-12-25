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

#include <string.h>

#include <gtest/gtest.h>

namespace traa {
namespace base {

// Run 900 times to mimic 1280x720.
// TODO(fbarchard): Remove benchmark once performance is non-issue.
static constexpr int k_times_to_run = 900;

static void generate_data(uint8_t *data, int size) {
  for (int i = 0; i < size; ++i) {
    data[i] = static_cast<uint8_t>(i);
  }
}

// Memory buffer large enough for 2 blocks aligned to 16 bytes.
static constexpr int k_size_of_block =
    k_differ_block_size * k_differ_block_size * k_differ_bytes_per_pixel;
uint8_t block_buffer[k_size_of_block * 2 + 16];

void prepare_buffers(uint8_t *&block1, uint8_t *&block2) {
  block1 = reinterpret_cast<uint8_t *>((reinterpret_cast<uintptr_t>(&block_buffer[0]) + 15) & ~15);
  generate_data(block1, k_size_of_block);
  block2 = block1 + k_size_of_block;
  memcpy(block2, block1, k_size_of_block);
}

TEST(block_difference_test_same, block_difference) {
  uint8_t *block1;
  uint8_t *block2;
  prepare_buffers(block1, block2);

  // These blocks should match.
  for (int i = 0; i < k_times_to_run; ++i) {
    int result = block_difference(block1, block2, k_differ_block_size * k_differ_bytes_per_pixel);
    EXPECT_EQ(0, result);
  }
}

TEST(block_difference_test_last, block_difference) {
  uint8_t *block1;
  uint8_t *block2;
  prepare_buffers(block1, block2);
  block2[k_size_of_block - 2] += 1;

  for (int i = 0; i < k_times_to_run; ++i) {
    int result = block_difference(block1, block2, k_differ_block_size * k_differ_bytes_per_pixel);
    EXPECT_EQ(1, result);
  }
}

TEST(block_difference_test_mid, block_difference) {
  uint8_t *block1;
  uint8_t *block2;
  prepare_buffers(block1, block2);
  block2[k_size_of_block / 2 + 1] += 1;

  for (int i = 0; i < k_times_to_run; ++i) {
    int result = block_difference(block1, block2, k_differ_block_size * k_differ_bytes_per_pixel);
    EXPECT_EQ(1, result);
  }
}

TEST(block_difference_test_first, block_difference) {
  uint8_t *block1;
  uint8_t *block2;
  prepare_buffers(block1, block2);
  block2[0] += 1;

  for (int i = 0; i < k_times_to_run; ++i) {
    int result = block_difference(block1, block2, k_differ_block_size * k_differ_bytes_per_pixel);
    EXPECT_EQ(1, result);
  }
}

} // namespace base
} // namespace traa
