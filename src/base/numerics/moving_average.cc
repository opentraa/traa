/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/numerics/moving_average.h"

#include "base/checks.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace traa {
namespace base {

moving_average::moving_average(size_t window_size) : history_(window_size, 0) {
  // Limit window size to avoid overflow.
  TRAA_DCHECK_LE(window_size, (int64_t{1} << 32) - 1);
}
moving_average::~moving_average() = default;

void moving_average::add_sample(int sample) {
  count_++;
  size_t index = count_ % history_.size();
  if (count_ > history_.size())
    sum_ -= history_[index];
  sum_ += sample;
  history_[index] = sample;
}

std::optional<int> moving_average::get_average_rounded_down() const {
  if (count_ == 0)
    return std::nullopt;
  return sum_ / size();
}

std::optional<int> moving_average::get_average_rounded_to_closest() const {
  if (count_ == 0)
    return std::nullopt;
  return (sum_ + size() / 2) / size();
}

std::optional<double> moving_average::get_unrounded_average() const {
  if (count_ == 0)
    return std::nullopt;
  return sum_ / static_cast<double>(size());
}

void moving_average::reset() {
  count_ = 0;
  sum_ = 0;
}

size_t moving_average::size() const { return std::min(count_, history_.size()); }

} // namespace base
} // namespace traa
