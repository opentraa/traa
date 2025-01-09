/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/screen_capturer_helper.h"

#include "base/checks.h"

namespace traa {
namespace base {

void screen_capturer_helper::clear_invalid_region() {
  std::lock_guard<std::mutex> scoped_invalid_region_lock(invalid_region_mutex_);
  invalid_region_.clear();
}

void screen_capturer_helper::invalidate_region(const desktop_region &invalid_region) {
  std::lock_guard<std::mutex> scoped_invalid_region_lock(invalid_region_mutex_);
  invalid_region_.add_region(invalid_region);
}

void screen_capturer_helper::invalidate_screen(const desktop_size &size) {
  std::lock_guard<std::mutex> scoped_invalid_region_lock(invalid_region_mutex_);
  invalid_region_.add_rect(desktop_rect::make_size(size));
}

void screen_capturer_helper::take_invalid_region(desktop_region *invalid_region) {
  invalid_region->clear();

  {
    std::lock_guard<std::mutex> scoped_invalid_region_lock(invalid_region_mutex_);
    invalid_region->swap(&invalid_region_);
  }

  if (log_grid_size_ > 0) {
    desktop_region expanded_region;
    expand_to_grid(*invalid_region, log_grid_size_, &expanded_region);
    expanded_region.swap(invalid_region);

    invalid_region->intersect_with(desktop_rect::make_size(size_most_recent_));
  }
}

void screen_capturer_helper::set_log_grid_size(int log_grid_size) {
  log_grid_size_ = log_grid_size;
}

const desktop_size &screen_capturer_helper::size_most_recent() const { return size_most_recent_; }

void screen_capturer_helper::set_size_most_recent(const desktop_size &size) {
  size_most_recent_ = size;
}

// Returns the largest multiple of `n` that is <= `x`.
// `n` must be a power of 2. `n_mask` is ~(`n` - 1).
static int down_to_multiple(int x, int n_mask) { return (x & n_mask); }

// Returns the smallest multiple of `n` that is >= `x`.
// `n` must be a power of 2. `n_mask` is ~(`n` - 1).
static int up_to_multiple(int x, int n, int n_mask) { return ((x + n - 1) & n_mask); }

void screen_capturer_helper::expand_to_grid(const desktop_region &region, int log_grid_size,
                                            desktop_region *result) {
  TRAA_DCHECK_GE(log_grid_size, 1);
  int grid_size = 1 << log_grid_size;
  int grid_size_mask = ~(grid_size - 1);

  result->clear();
  for (desktop_region::iterator it(region); !it.is_at_end(); it.advance()) {
    int left = down_to_multiple(it.rect().left(), grid_size_mask);
    int right = up_to_multiple(it.rect().right(), grid_size, grid_size_mask);
    int top = down_to_multiple(it.rect().top(), grid_size_mask);
    int bottom = up_to_multiple(it.rect().bottom(), grid_size, grid_size_mask);
    result->add_rect(desktop_rect::make_ltrb(left, top, right, bottom));
  }
}

} // namespace base
} // namespace traa
