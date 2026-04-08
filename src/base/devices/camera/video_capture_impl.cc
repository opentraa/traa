/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/camera/video_capture_impl.h"

#include "base/devices/camera/video_capture_defines.h"
#include "base/logger.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <libyuv.h>

namespace traa {
namespace base {

namespace {

// Nanoseconds per millisecond.
constexpr int64_t k_nanos_per_millisec = 1000000;
// Nanoseconds per microsecond.
constexpr int64_t k_nanos_per_microsec = 1000;

inline int64_t time_nanos() {
  return std::chrono::steady_clock::now().time_since_epoch().count();
}

inline int64_t time_millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

} // namespace

video_capture_impl::video_capture_impl()
    : device_unique_id_(nullptr), requested_capability_(), data_callback_(nullptr),
      raw_data_callback_(nullptr), rotate_frame_(0), apply_rotation_(false),
      last_process_time_nanos_(time_nanos()),
      last_frame_rate_callback_time_nanos_(time_nanos()),
      last_process_frame_time_nanos_(time_nanos()) {
  requested_capability_.width = k_default_width;
  requested_capability_.height = k_default_height;
  requested_capability_.max_fps = 30;
  requested_capability_.video_type = video_type::k_i420;
  memset(incoming_frame_times_nanos_, 0, sizeof(incoming_frame_times_nanos_));
}

video_capture_impl::~video_capture_impl() {
  deregister_capture_data_callback();
  if (device_unique_id_)
    delete[] device_unique_id_;
}

void video_capture_impl::register_capture_data_callback(video_frame_callback *callback) {
  std::lock_guard<std::mutex> lock(api_lock_);
  assert(!raw_data_callback_);
  data_callback_ = callback;
}

void video_capture_impl::register_capture_data_callback(raw_video_callback *callback) {
  std::lock_guard<std::mutex> lock(api_lock_);
  assert(!data_callback_);
  raw_data_callback_ = callback;
}

void video_capture_impl::deregister_capture_data_callback() {
  std::lock_guard<std::mutex> lock(api_lock_);
  data_callback_ = nullptr;
  raw_data_callback_ = nullptr;
}

int32_t video_capture_impl::incoming_frame(uint8_t *video_frame, size_t video_frame_length,
                                           const video_capture_capability &frame_info,
                                           int64_t capture_time) {
  std::lock_guard<std::mutex> lock(api_lock_);

  const int32_t width = frame_info.width;
  const int32_t height = frame_info.height;

  if (raw_data_callback_) {
    update_frame_count();
    raw_data_callback_->on_raw_frame(video_frame, video_frame_length, frame_info, rotate_frame_,
                                     capture_time);
    return 0;
  }

  // Not encoded, convert to I420.
  if (frame_info.video_type != video_type::k_mjpeg) {
    // Allow buffers larger than expected. On linux gstreamer allocates buffers
    // page-aligned and v4l2loopback passes us the buffer size verbatim which
    // for most cases is larger than expected.
    if (auto size = calc_buffer_size(frame_info.video_type, width, abs(height));
        video_frame_length < size) {
      LOG_ERROR("wrong incoming frame length. expected {}, got {}.", size, video_frame_length);
      return -1;
    }
  }

  int stride_y = width;
  int stride_uv = (width + 1) / 2;
  int target_width = width;
  int target_height = abs(height);

  if (apply_rotation_) {
    // Rotating resolution for 90/270 degree rotations.
    if (rotate_frame_ == 90 || rotate_frame_ == 270) {
      target_width = abs(height);
      target_height = width;
    }
  }

  // Allocate I420 buffer.
  // Y plane: target_width * target_height
  // U plane: ((target_width+1)/2) * ((target_height+1)/2)
  // V plane: same as U
  int dst_stride_y = target_width;
  int dst_stride_uv = (target_width + 1) / 2;
  int y_size = dst_stride_y * target_height;
  int uv_size = dst_stride_uv * ((target_height + 1) / 2);
  size_t i420_size = static_cast<size_t>(y_size + uv_size * 2);
  std::vector<uint8_t> i420_buffer(i420_size);

  uint8_t *dst_y = i420_buffer.data();
  uint8_t *dst_u = dst_y + y_size;
  uint8_t *dst_v = dst_u + uv_size;

  // Map rotation degrees to libyuv rotation mode.
  libyuv::RotationMode rotation_mode = libyuv::kRotate0;
  if (apply_rotation_) {
    switch (rotate_frame_) {
    case 0:
      rotation_mode = libyuv::kRotate0;
      break;
    case 90:
      rotation_mode = libyuv::kRotate90;
      break;
    case 180:
      rotation_mode = libyuv::kRotate180;
      break;
    case 270:
      rotation_mode = libyuv::kRotate270;
      break;
    }
  }

  const int conversion_result = libyuv::ConvertToI420(
      video_frame, video_frame_length, dst_y, dst_stride_y, dst_u, dst_stride_uv, dst_v,
      dst_stride_uv, 0, 0, // No cropping
      width, height, target_width, target_height, rotation_mode,
      convert_video_type(frame_info.video_type));

  if (conversion_result != 0) {
    LOG_ERROR("failed to convert capture frame from type {} to I420.",
              static_cast<int>(frame_info.video_type));
    return -1;
  }

  update_frame_count();

  int64_t timestamp_ms = time_millis();

  if (data_callback_) {
    data_callback_->on_frame(i420_buffer.data(), target_width, target_height, i420_size,
                             timestamp_ms);
  }

  return 0;
}

int32_t video_capture_impl::start_capture(const video_capture_capability &capability) {
  requested_capability_ = capability;
  return -1;
}

int32_t video_capture_impl::stop_capture() { return -1; }

bool video_capture_impl::capture_started() { return false; }

int32_t video_capture_impl::capture_settings(video_capture_capability & /*settings*/) {
  return -1;
}

int32_t video_capture_impl::set_capture_rotation(int rotation) {
  std::lock_guard<std::mutex> lock(api_lock_);
  if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
    return -1;
  }
  rotate_frame_ = rotation;
  return 0;
}

bool video_capture_impl::set_apply_rotation(bool enable) {
  std::lock_guard<std::mutex> lock(api_lock_);
  apply_rotation_ = enable;
  return true;
}

bool video_capture_impl::get_apply_rotation() {
  std::lock_guard<std::mutex> lock(api_lock_);
  return apply_rotation_;
}

const char *video_capture_impl::current_device_name() const { return device_unique_id_; }

void video_capture_impl::update_frame_count() {
  if (incoming_frame_times_nanos_[0] / k_nanos_per_microsec == 0) {
    // First frame, no shift needed.
  } else {
    // Shift the array to make room for the new timestamp.
    for (int i = (k_frame_rate_count_history_size - 2); i >= 0; --i) {
      incoming_frame_times_nanos_[i + 1] = incoming_frame_times_nanos_[i];
    }
  }
  incoming_frame_times_nanos_[0] = time_nanos();
}

uint32_t video_capture_impl::calculate_frame_rate(int64_t now_ns) {
  int32_t num = 0;
  int32_t nr_of_frames = 0;
  for (num = 1; num < (k_frame_rate_count_history_size - 1); ++num) {
    if (incoming_frame_times_nanos_[num] <= 0 ||
        (now_ns - incoming_frame_times_nanos_[num]) / k_nanos_per_millisec >
            k_frame_rate_history_window_ms) {
      break;
    } else {
      nr_of_frames++;
    }
  }
  if (num > 1) {
    int64_t diff = (now_ns - incoming_frame_times_nanos_[num - 1]) / k_nanos_per_millisec;
    if (diff > 0) {
      return static_cast<uint32_t>((nr_of_frames * 1000.0f / diff) + 0.5f);
    }
  }

  return nr_of_frames;
}

} // namespace base
} // namespace traa
