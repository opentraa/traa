/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURE_FRAME_QUEUE_H_
#define TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURE_FRAME_QUEUE_H_

#include <memory>

namespace traa {
namespace base {

// Represents a queue of reusable video frames. Provides access to the 'current'
// frame - the frame that the caller is working with at the moment, and to the
// 'previous' frame - the predecessor of the current frame swapped by
// move_to_next_frame() call, if any.
//
// The caller is expected to (re)allocate frames if current_frame() returns
// NULL. The caller can mark all frames in the queue for reallocation (when,
// say, frame dimensions change). The queue records which frames need updating
// which the caller can query.
//
// Frame consumer is expected to never hold more than k_queue_length frames
// created by this function and it should release the earliest one before trying
// to capture a new frame (i.e. before move_to_next_frame() is called).
template <typename frame_t> class screen_capture_frame_queue {
public:
  screen_capture_frame_queue() = default;
  ~screen_capture_frame_queue() = default;

  screen_capture_frame_queue(const screen_capture_frame_queue &) = delete;
  screen_capture_frame_queue &operator=(const screen_capture_frame_queue &) = delete;

  // Moves to the next frame in the queue, moving the 'current' frame to become
  // the 'previous' one.
  void move_to_next_frame() { current_ = (current_ + 1) % k_queue_length; }

  // Replaces the current frame with a new one allocated by the caller. The
  // existing frame (if any) is destroyed. Takes ownership of `frame`.
  void replace_current_frame(std::unique_ptr<frame_t> frame) {
    frames_[current_] = std::move(frame);
  }

  // Marks all frames obsolete and resets the previous frame pointer. No
  // frames are freed though as the caller can still access them.
  void reset() {
    for (int i = 0; i < k_queue_length; i++) {
      frames_[i].reset();
    }
    current_ = 0;
  }

  frame_t *current_frame() const { return frames_[current_].get(); }

  frame_t *previous_frame() const {
    return frames_[(current_ + k_queue_length - 1) % k_queue_length].get();
  }

private:
  // Index of the current frame.
  int current_ = 0;

  static constexpr int k_queue_length = 2;
  std::unique_ptr<frame_t> frames_[k_queue_length];
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_SCREEN_CAPTURE_FRAME_QUEUE_H_
