/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/scoped_thread_desktop.h"

#include "base/devices/screen/win/thread_desktop.h"

namespace traa {
namespace base {

scoped_thread_desktop::scoped_thread_desktop() : initial_(thread_desktop::get_thread_desktop()) {}

scoped_thread_desktop::~scoped_thread_desktop() { revert(); }

bool scoped_thread_desktop::is_same(const thread_desktop &other) const {
  if (assigned_.get() != nullptr) {
    return assigned_->is_same(other);
  } else {
    return initial_->is_same(other);
  }
}

void scoped_thread_desktop::revert() {
  if (assigned_.get() != NULL) {
    initial_->set_thread_desktop();
    assigned_.reset();
  }
}

bool scoped_thread_desktop::set_thread_desktop(thread_desktop *desktop) {
  revert();

  std::unique_ptr<thread_desktop> scoped_desktop(desktop);

  if (initial_->is_same(*desktop))
    return true;

  if (!desktop->set_thread_desktop())
    return false;

  assigned_.reset(scoped_desktop.release());
  return true;
}

} // namespace base
} // namespace traa
