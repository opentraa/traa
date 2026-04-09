/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_IMPL_H_
#define TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_IMPL_H_

#include "base/devices/camera/video_capture_config.h"
#include "base/devices/camera/video_capture_defines.h"

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace traa {
namespace base {

// Callback interface for receiving I420-converted video frames.
class video_frame_callback {
public:
  virtual ~video_frame_callback() = default;
  virtual void on_frame(const uint8_t *buffer, int32_t width, int32_t height, size_t length,
                        int64_t timestamp_ms) = 0;
};

// Callback interface for receiving raw (unconverted) video frames.
class raw_video_callback {
public:
  virtual ~raw_video_callback() = default;
  virtual void on_raw_frame(uint8_t *buffer, size_t length,
                            const video_capture_capability &frame_info, int32_t rotation,
                            int64_t capture_time) = 0;
};

class video_capture_impl {
public:
  video_capture_impl();
  virtual ~video_capture_impl();

  // Callback management.
  void register_capture_data_callback(video_frame_callback *callback);
  void register_capture_data_callback(raw_video_callback *callback);
  void deregister_capture_data_callback();

  // Rotation control.
  // `rotation` must be one of 0, 90, 180, 270.
  int32_t set_capture_rotation(int rotation);
  bool set_apply_rotation(bool enable);
  bool get_apply_rotation();

  // Called by platform subclasses to deliver a captured frame.
  // `capture_time` is specified in NTP time format in milliseconds.
  int32_t incoming_frame(uint8_t *video_frame, size_t video_frame_length,
                         const video_capture_capability &frame_info, int64_t capture_time = 0);

  // Platform-dependent methods to be overridden by subclasses.
  virtual int32_t start_capture(const video_capture_capability &capability);
  virtual int32_t stop_capture();
  virtual bool capture_started();
  virtual int32_t capture_settings(video_capture_capability &settings);

  const char *current_device_name() const;

protected:
  // Current device unique name.
  char *device_unique_id_ = nullptr;
  std::mutex api_lock_;
  // Should be set by platform-dependent code in start_capture.
  video_capture_capability requested_capability_;

private:
  void update_frame_count();
  uint32_t calculate_frame_rate(int64_t now_ns);

  video_frame_callback *data_callback_ = nullptr;  // guarded by api_lock_
  raw_video_callback *raw_data_callback_ = nullptr; // guarded by api_lock_
  int32_t rotate_frame_ = 0;                        // guarded by api_lock_
  bool apply_rotation_ = false;                     // guarded by api_lock_

  // Timestamp history for frame rate calculation.
  int64_t incoming_frame_times_nanos_[k_frame_rate_count_history_size] = {};
  int64_t last_process_time_nanos_ = 0;
  int64_t last_frame_rate_callback_time_nanos_ = 0;
  int64_t last_process_frame_time_nanos_ = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_IMPL_H_
