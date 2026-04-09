/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/camera/win/video_capture_ds.h"

namespace traa {
namespace base {

device_info_impl *create_device_info() { return device_info_ds::create(); }

video_capture_impl *create_video_capture(const char *device_id) {
  if (device_id == nullptr)
    return nullptr;

  auto *capture = new video_capture_ds();
  if (capture->init(device_id) != 0) {
    delete capture;
    return nullptr;
  }

  return capture;
}

} // namespace base
} // namespace traa
