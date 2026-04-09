/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_CONFIG_H_
#define TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_CONFIG_H_

namespace traa {
namespace base {

enum { k_default_width = 640 };     // Start width
enum { k_default_height = 480 };    // Start height
enum { k_default_frame_rate = 30 }; // Start frame rate

enum { k_max_frame_rate = 60 }; // Max allowed frame rate of the start image

enum { k_default_capture_delay = 120 };
enum {
  k_max_capture_delay = 270
}; // Max capture delay allowed in the precompiled capture delay values.

enum { k_frame_rate_callback_interval = 1000 };
enum { k_frame_rate_count_history_size = 90 };
enum { k_frame_rate_history_window_ms = 2000 };

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_CONFIG_H_
