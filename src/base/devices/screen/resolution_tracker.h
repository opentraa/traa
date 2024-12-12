/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_RESOLUTION_TRACKER_H_
#define TRAA_BASE_DEVICES_SCREEN_RESOLUTION_TRACKER_H_

#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

class resolution_tracker final {
public:
  // Sets the resolution to `size`. Returns true if a previous size was recorded
  // and differs from `size`.
  bool set_resolution(desktop_size size);

  // Resets to the initial state.
  void reset();

private:
  desktop_size last_size_;
  bool initialized_ = false;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_RESOLUTION_TRACKER_H_
