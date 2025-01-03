/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURER_HELPER_H_
#define TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURER_HELPER_H_

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/thread_annotations.h"

#include <memory>
#include <mutex>

namespace traa {
namespace base {

// screen_capturer_helper is intended to be used by an implementation of the
// screen_capturer interface. It maintains a thread-safe invalid region, and
// the size of the most recently captured screen, on behalf of the
// screen_capturer that owns it.
class screen_capturer_helper {
public:
  screen_capturer_helper() = default;
  ~screen_capturer_helper() = default;

  screen_capturer_helper(const screen_capturer_helper &) = delete;
  screen_capturer_helper &operator=(const screen_capturer_helper &) = delete;

  // Clear out the invalid region.
  void clear_invalid_region();

  // Invalidate the specified region.
  void invalidate_region(const desktop_region &invalid_region);

  // Invalidate the entire screen, of a given size.
  void invalidate_screen(const desktop_size &size);

  // Copies current invalid region to `invalid_region` clears invalid region
  // storage for the next frame.
  void take_invalid_region(desktop_region *invalid_region);

  // Access the size of the most recently captured screen.
  const desktop_size &size_most_recent() const;
  void set_size_most_recent(const desktop_size &size);

  // Lossy compression can result in color values leaking between pixels in one
  // block. If part of a block changes, then unchanged parts of that block can
  // be changed in the compressed output. So we need to re-render an entire
  // block whenever part of the block changes.
  //
  // If `log_grid_size` is >= 1, then this function makes TakeInvalidRegion()
  // produce an invalid region expanded so that its vertices lie on a grid of
  // size 2 ^ `log_grid_size`. The expanded region is then clipped to the size
  // of the most recently captured screen, as previously set by
  // set_size_most_recent().
  // If `log_grid_size` is <= 0, then the invalid region is not expanded.
  void set_log_grid_size(int log_grid_size);

  // Expands a region so that its vertices all lie on a grid.
  // The grid size must be >= 2, so `log_grid_size` must be >= 1.
  static void expand_to_grid(const desktop_region &region, int log_grid_size,
                             desktop_region *result);

private:
  // A region that has been manually invalidated (through InvalidateRegion).
  // These will be returned as dirty_region in the capture data during the next
  // capture.
  desktop_region invalid_region_ TRAA_GUARDED_BY(invalid_region_mutex_);

  // A lock protecting `invalid_region_` across threads.
  std::mutex invalid_region_mutex_;

  // The size of the most recently captured screen.
  desktop_size size_most_recent_;

  // The log (base 2) of the size of the grid to which the invalid region is
  // expanded.
  // If the value is <= 0, then the invalid region is not expanded to a grid.
  int log_grid_size_ = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURER_HELPER_H_
