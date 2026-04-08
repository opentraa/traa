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

#include <dvdmedia.h> // VIDEOINFOHEADER2

#include "base/devices/camera/video_capture_config.h"
#include "base/devices/camera/win/help_functions_ds.h"
#include "base/devices/camera/win/sink_filter_ds.h"
#include "base/logger.h"

namespace traa {
namespace base {

video_capture_ds::video_capture_ds()
    : capture_filter_(nullptr), graph_builder_(nullptr), media_control_(nullptr),
      sink_filter_(nullptr), input_send_pin_(nullptr), output_capture_pin_(nullptr),
      dv_filter_(nullptr), input_dv_pin_(nullptr), output_dv_pin_(nullptr) {}

video_capture_ds::~video_capture_ds() {
  if (media_control_) {
    media_control_->Stop();
  }
  if (graph_builder_) {
    if (sink_filter_)
      graph_builder_->RemoveFilter(static_cast<IBaseFilter *>(sink_filter_));
    if (capture_filter_)
      graph_builder_->RemoveFilter(capture_filter_);
    if (dv_filter_)
      graph_builder_->RemoveFilter(dv_filter_);
  }
  RELEASE_AND_CLEAR(input_send_pin_);
  RELEASE_AND_CLEAR(output_capture_pin_);

  RELEASE_AND_CLEAR(capture_filter_);
  RELEASE_AND_CLEAR(dv_filter_);

  RELEASE_AND_CLEAR(media_control_);

  RELEASE_AND_CLEAR(input_dv_pin_);
  RELEASE_AND_CLEAR(output_dv_pin_);

  RELEASE_AND_CLEAR(sink_filter_);

  RELEASE_AND_CLEAR(graph_builder_);
}

int32_t video_capture_ds::init(const char *device_unique_id_utf8) {
  const int32_t name_length = static_cast<int32_t>(strlen(device_unique_id_utf8));
  if (name_length >= k_video_capture_unique_name_length)
    return -1;

  // Store the device name.
  device_unique_id_ = new (std::nothrow) char[name_length + 1];
  memcpy(device_unique_id_, device_unique_id_utf8, name_length + 1);

  if (ds_info_.init() != 0)
    return -1;

  capture_filter_ = ds_info_.get_device_filter(device_unique_id_utf8);
  if (!capture_filter_) {
    LOG_INFO("failed to create capture filter.");
    return -1;
  }

  // Get the interface for DirectShow's GraphBuilder.
  HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder,
                                reinterpret_cast<void **>(&graph_builder_));
  if (FAILED(hr)) {
    LOG_INFO("failed to create graph builder.");
    return -1;
  }

  hr = graph_builder_->QueryInterface(IID_IMediaControl,
                                      reinterpret_cast<void **>(&media_control_));
  if (FAILED(hr)) {
    LOG_INFO("failed to create media control builder.");
    return -1;
  }

  hr = graph_builder_->AddFilter(capture_filter_, CAPTURE_FILTER_NAME);
  if (FAILED(hr)) {
    LOG_INFO("failed to add the capture device to the graph.");
    return -1;
  }

  output_capture_pin_ = get_output_pin(capture_filter_, PIN_CATEGORY_CAPTURE);
  if (!output_capture_pin_) {
    LOG_INFO("failed to get output capture pin");
    return -1;
  }

  // Create the sink filter used for receiving captured frames.
  sink_filter_ = new com_ref_count<capture_sink_filter>(this);

  hr = graph_builder_->AddFilter(static_cast<IBaseFilter *>(sink_filter_), SINK_FILTER_NAME);
  if (FAILED(hr)) {
    LOG_INFO("failed to add the send filter to the graph.");
    return -1;
  }

  input_send_pin_ = get_input_pin(static_cast<IBaseFilter *>(sink_filter_));
  if (!input_send_pin_) {
    LOG_INFO("failed to get input send pin");
    return -1;
  }

  if (set_camera_output(requested_capability_) != 0) {
    return -1;
  }

  LOG_INFO("capture device '{}' initialized.", device_unique_id_utf8);
  return 0;
}

int32_t video_capture_ds::start_capture(const video_capture_capability &capability) {
  if (capability != requested_capability_) {
    disconnect_graph();

    if (set_camera_output(capability) != 0) {
      return -1;
    }
  }

  HRESULT hr = media_control_->Pause();
  if (FAILED(hr)) {
    LOG_INFO("failed to Pause the capture device. Is it already occupied? hr=0x{:08x}",
             static_cast<unsigned>(hr));
    return -1;
  }

  hr = media_control_->Run();
  if (FAILED(hr)) {
    LOG_INFO("failed to start the capture device.");
    return -1;
  }
  return 0;
}

int32_t video_capture_ds::stop_capture() {
  HRESULT hr = media_control_->StopWhenReady();
  if (FAILED(hr)) {
    LOG_INFO("failed to stop the capture graph. hr=0x{:08x}", static_cast<unsigned>(hr));
    return -1;
  }
  return 0;
}

bool video_capture_ds::capture_started() {
  OAFilterState state = 0;
  HRESULT hr = media_control_->GetState(1000, &state);
  if (hr != S_OK && hr != VFW_S_CANT_CUE) {
    LOG_INFO("failed to get the capture_started status");
  }
  LOG_INFO("capture_started state={}", state);
  return state == State_Running;
}

int32_t video_capture_ds::capture_settings(video_capture_capability &settings) {
  settings = requested_capability_;
  return 0;
}

int32_t
video_capture_ds::set_camera_output(const video_capture_capability &requested_capability) {
  // Get the best matching capability.
  video_capture_capability capability;
  int32_t capability_index;

  // Store the new requested size.
  requested_capability_ = requested_capability;

  // Match the requested capability with the supported.
  capability_index =
      ds_info_.get_best_matched_capability(device_unique_id_, requested_capability_, capability);
  if (capability_index < 0) {
    return -1;
  }

  // Reduce the frame rate if possible.
  if (capability.max_fps > requested_capability.max_fps) {
    capability.max_fps = requested_capability.max_fps;
  } else if (capability.max_fps <= 0) {
    capability.max_fps = 30;
  }

  // Convert it to the windows capability index since they are not necessarily the same.
  video_capture_capability_windows windows_capability;
  if (ds_info_.get_windows_capability(capability_index, windows_capability) != 0) {
    return -1;
  }

  IAMStreamConfig *stream_config = nullptr;
  AM_MEDIA_TYPE *pmt = nullptr;
  VIDEO_STREAM_CONFIG_CAPS caps;

  HRESULT hr = output_capture_pin_->QueryInterface(IID_IAMStreamConfig,
                                                   reinterpret_cast<void **>(&stream_config));
  if (hr) {
    LOG_INFO("can't get the capture format settings.");
    return -1;
  }

  // Get the windows capability from the capture device.
  bool is_dv_camera = false;
  hr = stream_config->GetStreamCaps(windows_capability.direct_show_capability_index, &pmt,
                                    reinterpret_cast<BYTE *>(&caps));
  if (hr == S_OK) {
    if (pmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2 *h = reinterpret_cast<VIDEOINFOHEADER2 *>(pmt->pbFormat);
      if (capability.max_fps > 0 && windows_capability.support_frame_rate_control) {
        h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.max_fps);
      }
    } else {
      VIDEOINFOHEADER *h = reinterpret_cast<VIDEOINFOHEADER *>(pmt->pbFormat);
      if (capability.max_fps > 0 && windows_capability.support_frame_rate_control) {
        h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.max_fps);
      }
    }

    // Set the sink filter to request this capability.
    sink_filter_->set_requested_capability(capability);

    // Order the capture device to use this capability.
    hr += stream_config->SetFormat(pmt);

    // Check if this is a DV camera and we need to add MS DV Filter.
    if (pmt->subtype == MEDIASUBTYPE_dvsl || pmt->subtype == MEDIASUBTYPE_dvsd ||
        pmt->subtype == MEDIASUBTYPE_dvhd) {
      is_dv_camera = true;
    }

    free_media_type(pmt);
    pmt = nullptr;
  }
  RELEASE_AND_CLEAR(stream_config);

  if (FAILED(hr)) {
    LOG_INFO("failed to set capture device output format");
    return -1;
  }

  if (is_dv_camera) {
    hr = connect_dv_camera();
  } else {
    hr = graph_builder_->ConnectDirect(output_capture_pin_, input_send_pin_, NULL);
  }
  if (hr != S_OK) {
    LOG_INFO("failed to connect the capture graph hr=0x{:08x}", static_cast<unsigned>(hr));
    return -1;
  }
  return 0;
}

int32_t video_capture_ds::disconnect_graph() {
  HRESULT hr = media_control_->Stop();
  hr += graph_builder_->Disconnect(output_capture_pin_);
  hr += graph_builder_->Disconnect(input_send_pin_);

  // If the DV camera filter exists.
  if (dv_filter_) {
    graph_builder_->Disconnect(input_dv_pin_);
    graph_builder_->Disconnect(output_dv_pin_);
  }
  if (hr != S_OK) {
    LOG_ERROR("failed to stop the capture device for reconfiguration hr=0x{:08x}",
              static_cast<unsigned>(hr));
    return -1;
  }
  return 0;
}

HRESULT video_capture_ds::connect_dv_camera() {
  HRESULT hr = S_OK;

  if (!dv_filter_) {
    hr = CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC, IID_IBaseFilter,
                          reinterpret_cast<void **>(&dv_filter_));
    if (hr != S_OK) {
      LOG_INFO("failed to create the dv decoder: hr=0x{:08x}", static_cast<unsigned>(hr));
      return hr;
    }
    hr = graph_builder_->AddFilter(dv_filter_, L"VideoDecoderDV");
    if (hr != S_OK) {
      LOG_INFO("failed to add the dv decoder to the graph: hr=0x{:08x}",
               static_cast<unsigned>(hr));
      return hr;
    }
    input_dv_pin_ = get_input_pin(dv_filter_);
    if (input_dv_pin_ == nullptr) {
      LOG_INFO("failed to get input pin from DV decoder");
      return -1;
    }
    output_dv_pin_ = get_output_pin(dv_filter_, GUID_NULL);
    if (output_dv_pin_ == nullptr) {
      LOG_INFO("failed to get output pin from DV decoder");
      return -1;
    }
  }

  hr = graph_builder_->ConnectDirect(output_capture_pin_, input_dv_pin_, NULL);
  if (hr != S_OK) {
    LOG_INFO("failed to connect capture device to the dv decoder: hr=0x{:08x}",
             static_cast<unsigned>(hr));
    return hr;
  }

  hr = graph_builder_->ConnectDirect(output_dv_pin_, input_send_pin_, NULL);
  if (hr != S_OK) {
    if (hr == HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES)) {
      LOG_INFO("failed to connect the capture device, busy");
    } else {
      LOG_INFO("failed to connect capture device to the send graph: hr=0x{:08x}",
               static_cast<unsigned>(hr));
    }
  }
  return hr;
}

} // namespace base
} // namespace traa
