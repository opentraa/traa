/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_DEVICE_INFO_IMPL_H_
#define TRAA_BASE_DEVICES_CAMERA_DEVICE_INFO_IMPL_H_

#include "base/devices/camera/video_capture_defines.h"

#include <cstdint>
#include <mutex>
#include <vector>

namespace traa {
namespace base {

class device_info_impl {
public:
  device_info_impl();
  virtual ~device_info_impl();

  int32_t number_of_capabilities(const char *device_unique_id_utf8);
  int32_t get_capability(const char *device_unique_id_utf8, uint32_t device_capability_number,
                         video_capture_capability &capability);
  int32_t get_best_matched_capability(const char *device_unique_id_utf8,
                                      const video_capture_capability &requested,
                                      video_capture_capability &resulting);

  virtual uint32_t number_of_devices() = 0;
  virtual int32_t get_device_name(uint32_t device_number, char *device_name_utf8,
                                  uint32_t device_name_length, char *device_unique_id_utf8,
                                  uint32_t device_unique_id_utf8_length,
                                  char *product_unique_id_utf8,
                                  uint32_t product_unique_id_utf8_length) = 0;

protected:
  virtual int32_t init() = 0;
  virtual int32_t create_capability_map(const char *device_unique_id_utf8) = 0;

  using video_capture_capabilities = std::vector<video_capture_capability>;
  video_capture_capabilities capture_capabilities_; // guarded by api_lock_
  std::mutex api_lock_;
  char *last_used_device_name_ = nullptr;     // guarded by api_lock_
  uint32_t last_used_device_name_length_ = 0; // guarded by api_lock_
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_DEVICE_INFO_IMPL_H_
