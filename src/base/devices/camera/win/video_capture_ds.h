/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_WIN_VIDEO_CAPTURE_DS_H_
#define TRAA_BASE_DEVICES_CAMERA_WIN_VIDEO_CAPTURE_DS_H_

#include "base/devices/camera/video_capture_impl.h"
#include "base/devices/camera/win/device_info_ds.h"

#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME L"SinkFilter"

namespace traa {
namespace base {

// Forward declaration.
class capture_sink_filter;

class video_capture_ds : public video_capture_impl {
public:
  video_capture_ds();

  virtual int32_t init(const char *device_unique_id_utf8);

  // Start/Stop
  int32_t start_capture(const video_capture_capability &capability) override;
  int32_t stop_capture() override;

  // Properties of the set device
  bool capture_started() override;
  int32_t capture_settings(video_capture_capability &settings) override;

  ~video_capture_ds() override;

private:
  int32_t set_camera_output(const video_capture_capability &requested_capability);
  int32_t disconnect_graph();
  HRESULT connect_dv_camera();

  device_info_ds ds_info_;

  IBaseFilter *capture_filter_ = nullptr;
  IGraphBuilder *graph_builder_ = nullptr;
  IMediaControl *media_control_ = nullptr;
  capture_sink_filter *sink_filter_ = nullptr;
  IPin *input_send_pin_ = nullptr;
  IPin *output_capture_pin_ = nullptr;

  // Microsoft DV interface (external DV cameras)
  IBaseFilter *dv_filter_ = nullptr;
  IPin *input_dv_pin_ = nullptr;
  IPin *output_dv_pin_ = nullptr;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_WIN_VIDEO_CAPTURE_DS_H_
