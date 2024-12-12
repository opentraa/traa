/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/cropped_desktop_frame.h"
#include "base/devices/screen/desktop_region.h"

#include <memory>
#include <utility>

namespace traa {
namespace base {

// A desktop_frame that is a sub-rect of another desktop_frame.
class cropped_desktop_frame : public desktop_frame {
public:
  cropped_desktop_frame(std::unique_ptr<desktop_frame> frame, const desktop_rect &rect);

  cropped_desktop_frame(const cropped_desktop_frame &) = delete;
  cropped_desktop_frame &operator=(const cropped_desktop_frame &) = delete;

private:
  const std::unique_ptr<desktop_frame> frame_;
};

std::unique_ptr<desktop_frame> create_cropped_desktop_frame(std::unique_ptr<desktop_frame> frame,
                                                            const desktop_rect &rect) {

  desktop_rect intersection = desktop_rect::make_size(frame->size());
  intersection.intersect_with(rect);
  if (intersection.is_empty()) {
    return nullptr;
  }

  if (frame->size().equals(rect.size())) {
    return frame;
  }

  return std::unique_ptr<desktop_frame>(new cropped_desktop_frame(std::move(frame), intersection));
}

cropped_desktop_frame::cropped_desktop_frame(std::unique_ptr<desktop_frame> frame,
                                             const desktop_rect &rect)
    : desktop_frame(rect.size(), frame->stride(), frame->get_frame_data_at_pos(rect.top_left()),
                    frame->get_shared_memory()),
      frame_(std::move(frame)) {
  move_frame_info_from(frame_.get());
  set_top_left(frame_->top_left().add(rect.top_left()));
  mutable_updated_region()->intersect_with(rect);
  mutable_updated_region()->translate(-rect.left(), -rect.top());
}

} // namespace base
} // namespace traa
