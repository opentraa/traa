/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/shared_desktop_frame.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace traa {
namespace base {

shared_desktop_frame::~shared_desktop_frame() {}

// static
std::unique_ptr<shared_desktop_frame>
shared_desktop_frame::wrap(std::unique_ptr<desktop_frame> frame) {
  return std::unique_ptr<shared_desktop_frame>(
      new shared_desktop_frame(std::make_shared<std::unique_ptr<desktop_frame>>(std::move(frame))));
}

shared_desktop_frame *shared_desktop_frame::wrap(desktop_frame *frame) {
  return wrap(std::unique_ptr<desktop_frame>(frame)).release();
}

desktop_frame *shared_desktop_frame::get_underlying_frame() { return shared_core_->get(); }

bool shared_desktop_frame::share_frame_with(const shared_desktop_frame &other) const {
  return shared_core_->get() == other.shared_core_->get();
}

std::unique_ptr<shared_desktop_frame> shared_desktop_frame::share() {
  std::unique_ptr<shared_desktop_frame> result(new shared_desktop_frame(shared_core_));
  result->copy_frame_info_from(*this);
  return result;
}

bool shared_desktop_frame::is_shared() { return !shared_core_.unique(); }

shared_desktop_frame::shared_desktop_frame(shared_core_t core)
    : desktop_frame((*core)->size(), (*core)->stride(), (*core)->data(),
                    (*core)->get_shared_memory()),
      shared_core_(core) {
  copy_frame_info_from(*(shared_core_->get()));
}

} // namespace base
} // namespace traa
