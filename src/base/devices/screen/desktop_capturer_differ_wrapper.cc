/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer_differ_wrapper.h"

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/differ_block.h"
#include "base/utils/time_utils.h"

#include <stdint.h>
#include <string.h>
#include <utility>

namespace traa {
namespace base {

inline namespace {

// Returns true if (0, 0) - (`width`, `height`) vector in `old_buffer` and
// `new_buffer` are equal. `width` should be less than 32
// (defined by k_differ_block_size), otherwise block_difference() should be used.
bool partial_block_difference(const uint8_t *old_buffer, const uint8_t *new_buffer, int width,
                              int height, int stride) {
  const int width_bytes = width * desktop_frame::k_bytes_per_pixel;
  for (int i = 0; i < height; i++) {
    if (memcmp(old_buffer, new_buffer, width_bytes) != 0) {
      return true;
    }
    old_buffer += stride;
    new_buffer += stride;
  }
  return false;
}

// Compares columns in the range of [`left`, `right`), in a row in the
// range of [`top`, `top` + `height`), starts from `old_buffer` and
// `new_buffer`, and outputs updated regions into `output`. `stride` is the
// desktop_frame::stride().
void compare_row(const uint8_t *old_buffer, const uint8_t *new_buffer, const int left,
                 const int right, const int top, const int bottom, const int stride,
                 desktop_region *const output) {
  const int block_x_offset = k_differ_block_size * desktop_frame::k_bytes_per_pixel;
  const int width = right - left;
  const int height = bottom - top;
  const int block_count = (width - 1) / k_differ_block_size;
  const int last_block_width = width - block_count * k_differ_block_size;

  // The first block-column in a continuous dirty area in current block-row.
  int first_dirty_x_block = -1;

  // We always need to add dirty area into `output` in the last block, so handle
  // it separatedly.
  for (int x = 0; x < block_count; x++) {
    if (block_difference(old_buffer, new_buffer, height, stride)) {
      if (first_dirty_x_block == -1) {
        // This is the first dirty block in a continuous dirty area.
        first_dirty_x_block = x;
      }
    } else if (first_dirty_x_block != -1) {
      // The block on the left is the last dirty block in a continuous
      // dirty area.
      output->add_rect(desktop_rect::make_ltrb(first_dirty_x_block * k_differ_block_size + left,
                                               top, x * k_differ_block_size + left, bottom));
      first_dirty_x_block = -1;
    }
    old_buffer += block_x_offset;
    new_buffer += block_x_offset;
  }

  bool last_block_diff;
  if (last_block_width < k_differ_block_size) {
    // The last one is a partial vector.
    last_block_diff =
        partial_block_difference(old_buffer, new_buffer, last_block_width, height, stride);
  } else {
    last_block_diff = block_difference(old_buffer, new_buffer, height, stride);
  }
  if (last_block_diff) {
    if (first_dirty_x_block == -1) {
      first_dirty_x_block = block_count;
    }
    output->add_rect(desktop_rect::make_ltrb(first_dirty_x_block * k_differ_block_size + left, top,
                                             right, bottom));
  } else if (first_dirty_x_block != -1) {
    output->add_rect(desktop_rect::make_ltrb(first_dirty_x_block * k_differ_block_size + left, top,
                                             block_count * k_differ_block_size + left, bottom));
  }
}

// Compares `rect` area in `old_frame` and `new_frame`, and outputs dirty
// regions into `output`.
void compare_frames(const desktop_frame &old_frame, const desktop_frame &new_frame,
                    desktop_rect rect, desktop_region *const output) {
  rect.intersect_with(desktop_rect::make_size(old_frame.size()));

  const int y_block_count = (rect.height() - 1) / k_differ_block_size;
  const int last_y_block_height = rect.height() - y_block_count * k_differ_block_size;
  // Offset from the start of one block-row to the next.
  const int block_y_stride = old_frame.stride() * k_differ_block_size;
  const uint8_t *prev_block_row_start = old_frame.get_frame_data_at_pos(rect.top_left());
  const uint8_t *curr_block_row_start = new_frame.get_frame_data_at_pos(rect.top_left());

  int top = rect.top();
  // The last row may have a different height, so we handle it separately.
  for (int y = 0; y < y_block_count; y++) {
    compare_row(prev_block_row_start, curr_block_row_start, rect.left(), rect.right(), top,
                top + k_differ_block_size, old_frame.stride(), output);
    top += k_differ_block_size;
    prev_block_row_start += block_y_stride;
    curr_block_row_start += block_y_stride;
  }
  compare_row(prev_block_row_start, curr_block_row_start, rect.left(), rect.right(), top,
              top + last_y_block_height, old_frame.stride(), output);
}

} // namespace

desktop_capturer_differ_wrapper::desktop_capturer_differ_wrapper(
    std::unique_ptr<desktop_capturer> base_capturer)
    : base_capturer_(std::move(base_capturer)) {}

desktop_capturer_differ_wrapper::~desktop_capturer_differ_wrapper() {}

void desktop_capturer_differ_wrapper::start(desktop_capturer::capture_callback *callback) {
  callback_ = callback;
  base_capturer_->start(this);
}

void desktop_capturer_differ_wrapper::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> shared_memory_factory) {
  base_capturer_->set_shared_memory_factory(std::move(shared_memory_factory));
}

void desktop_capturer_differ_wrapper::capture_frame() { base_capturer_->capture_frame(); }

void desktop_capturer_differ_wrapper::set_excluded_window(win_id_t window) {
  base_capturer_->set_excluded_window(window);
}

bool desktop_capturer_differ_wrapper::get_source_list(source_list_t *sources) {
  return base_capturer_->get_source_list(sources);
}

bool desktop_capturer_differ_wrapper::select_source(source_id_t id) {
  return base_capturer_->select_source(id);
}

bool desktop_capturer_differ_wrapper::focus_on_selected_source() {
  return base_capturer_->focus_on_selected_source();
}

bool desktop_capturer_differ_wrapper::is_occluded(const desktop_vector &pos) {
  return base_capturer_->is_occluded(pos);
}

#if defined(TRAA_ENABLE_WAYLAND)
desktop_capture_metadata desktop_capturer_differ_wrapper::get_metadata() {
  return base_capturer_->get_metadata();
}
#endif // defined(TRAA_ENABLE_WAYLAND)

void desktop_capturer_differ_wrapper::on_capture_result(
    capture_result result, std::unique_ptr<desktop_frame> input_frame) {
  int64_t start_time_nanos = time_nanos();
  if (!input_frame) {
    callback_->on_capture_result(result, nullptr);
    return;
  }

  std::unique_ptr<shared_desktop_frame> frame = shared_desktop_frame::wrap(std::move(input_frame));
  if (last_frame_ && (last_frame_->size().width() != frame->size().width() ||
                      last_frame_->size().height() != frame->size().height() ||
                      last_frame_->stride() != frame->stride())) {
    last_frame_.reset();
  }

  if (last_frame_) {
    desktop_region hints;
    hints.swap(frame->mutable_updated_region());
    for (desktop_region::iterator it(hints); !it.is_at_end(); it.advance()) {
      compare_frames(*last_frame_, *frame, it.rect(), frame->mutable_updated_region());
    }
  } else {
    frame->mutable_updated_region()->set_rect(desktop_rect::make_size(frame->size()));
  }
  last_frame_ = frame->share();

  frame->set_capture_time_ms(frame->capture_time_ms() +
                             (time_nanos() - start_time_nanos) / k_num_nanosecs_per_millisec);
  callback_->on_capture_result(result, std::move(frame));
}

} // namespace base
} // namespace traa
