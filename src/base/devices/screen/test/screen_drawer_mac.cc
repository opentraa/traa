/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// TODO(zijiehe): Implement ScreenDrawerMac

#include "base/devices/screen/test/screen_drawer.h"
#include "base/devices/screen/test/screen_drawer_lock_posix.h"

#include <memory>

namespace traa {
namespace base {

// static
std::unique_ptr<screen_drawer_lock> screen_drawer_lock::create() {
  return std::make_unique<screen_drawer_lock_posix>();
}

// static
std::unique_ptr<screen_drawer> screen_drawer::create() { return nullptr; }

} // namespace base
} // namespace traa
