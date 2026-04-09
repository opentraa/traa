/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_WIN_DEVICE_INFO_DS_H_
#define TRAA_BASE_DEVICES_CAMERA_WIN_DEVICE_INFO_DS_H_

#include <dshow.h>

#include "base/devices/camera/device_info_impl.h"
#include "base/devices/camera/video_capture_impl.h"

#include <vector>

namespace traa {
namespace base {

struct video_capture_capability_windows : public video_capture_capability {
  uint32_t direct_show_capability_index = 0;
  bool support_frame_rate_control = false;
};

class device_info_ds : public device_info_impl {
public:
  // Factory function.
  static device_info_ds *create();

  device_info_ds();
  ~device_info_ds() override;

  int32_t init() override;
  uint32_t number_of_devices() override;

  // Returns the available capture devices.
  int32_t get_device_name(uint32_t device_number, char *device_name_utf8,
                          uint32_t device_name_length, char *device_unique_id_utf8,
                          uint32_t device_unique_id_utf8_length, char *product_unique_id_utf8,
                          uint32_t product_unique_id_utf8_length) override;

  // Windows specific

  // Gets a capture device filter.
  // The caller is responsible for releasing the filter when no longer needed.
  IBaseFilter *get_device_filter(const char *device_unique_id_utf8,
                                 char *product_unique_id_utf8 = nullptr,
                                 uint32_t product_unique_id_utf8_length = 0);

  int32_t get_windows_capability(int32_t capability_index,
                                 video_capture_capability_windows &windows_capability);

  static void get_product_id(const char *device_path, char *product_unique_id_utf8,
                             uint32_t product_unique_id_utf8_length);

protected:
  int32_t create_capability_map(const char *device_unique_id_utf8) override;

private:
  int32_t get_device_info(uint32_t device_number, char *device_name_utf8,
                          uint32_t device_name_length, char *device_unique_id_utf8,
                          uint32_t device_unique_id_utf8_length, char *product_unique_id_utf8,
                          uint32_t product_unique_id_utf8_length);

  ICreateDevEnum *ds_dev_enum_ = nullptr;
  IEnumMoniker *ds_moniker_dev_enum_ = nullptr;
  bool co_uninitialize_is_required_ = true;
  std::vector<video_capture_capability_windows> capture_capabilities_windows_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_WIN_DEVICE_INFO_DS_H_
