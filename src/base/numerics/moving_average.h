/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_NUMERICS_MOVING_AVERAGE_H_
#define TRAA_BASE_NUMERICS_MOVING_AVERAGE_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <vector>

namespace traa {
namespace base {

// Calculates average over fixed size window. If there are less than window
// size elements, calculates average of all inserted so far elements.
//
class moving_average {
public:
  // Maximum supported window size is 2^32 - 1.
  explicit moving_average(size_t window_size);
  ~moving_average();
  // moving_average is neither copyable nor movable.
  moving_average(const moving_average &) = delete;
  moving_average &operator=(const moving_average &) = delete;

  // Adds new sample. If the window is full, the oldest element is pushed out.
  void add_sample(int sample);

  // Returns rounded down average of last `window_size` elements or all
  // elements if there are not enough of them. Returns nullopt if there were
  // no elements added.
  std::optional<int> get_average_rounded_down() const;

  // Same as above but rounded to the closest integer.
  std::optional<int> get_average_rounded_to_closest() const;

  // Returns unrounded average over the window.
  std::optional<double> get_unrounded_average() const;

  // Resets to the initial state before any elements were added.
  void reset();

  // Returns number of elements in the window.
  size_t size() const;

private:
  // Total number of samples added to the class since last reset.
  size_t count_ = 0;
  // Sum of the samples in the moving window.
  int64_t sum_ = 0;
  // Circular buffer for all the samples in the moving window.
  // Size is always `window_size`
  std::vector<int> history_;
};

} // namespace base
} // namespace traa
#endif // TRAA_BASE_NUMERICS_MOVING_AVERAGE_H_
