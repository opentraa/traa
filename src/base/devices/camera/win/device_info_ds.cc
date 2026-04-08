/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/camera/win/device_info_ds.h"

#include <dvdmedia.h>

#include "base/devices/camera/video_capture_config.h"
#include "base/devices/camera/win/help_functions_ds.h"
#include "base/logger.h"

#include <cassert>
#include <cstring>
#include <mutex>

namespace traa {
namespace base {

// static
device_info_ds *device_info_ds::create() {
  device_info_ds *ds_info = new device_info_ds();
  if (!ds_info || ds_info->init() != 0) {
    delete ds_info;
    ds_info = nullptr;
  }
  return ds_info;
}

device_info_ds::device_info_ds()
    : ds_dev_enum_(nullptr), ds_moniker_dev_enum_(nullptr),
      co_uninitialize_is_required_(true) {
  // Initialize the COM library.
  // CoInitializeEx must be called at least once, and is usually called only
  // once, for each thread that uses the COM library.
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    // Avoid calling CoUninitialize() since CoInitializeEx() failed.
    co_uninitialize_is_required_ = false;

    if (hr == RPC_E_CHANGED_MODE) {
      // Calling thread has already initialized COM to be used in a
      // single-threaded apartment (STA).
      LOG_INFO("{}: CoInitializeEx(nullptr, COINIT_MULTITHREADED)"
               " => RPC_E_CHANGED_MODE, error 0x{:x}",
               __FUNCTION__, static_cast<unsigned long>(hr));
    }
  }
}

device_info_ds::~device_info_ds() {
  RELEASE_AND_CLEAR(ds_moniker_dev_enum_);
  RELEASE_AND_CLEAR(ds_dev_enum_);
  if (co_uninitialize_is_required_) {
    CoUninitialize();
  }
}

int32_t device_info_ds::init() {
  HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC,
                                IID_ICreateDevEnum, (void **)&ds_dev_enum_);
  if (hr != NOERROR) {
    LOG_INFO("Failed to create CLSID_SystemDeviceEnum, error 0x{:x}",
             static_cast<unsigned long>(hr));
    return -1;
  }
  return 0;
}

uint32_t device_info_ds::number_of_devices() {
  std::lock_guard<std::mutex> lock(api_lock_);
  return get_device_info(0, 0, 0, 0, 0, 0, 0);
}

int32_t device_info_ds::get_device_name(uint32_t device_number, char *device_name_utf8,
                                        uint32_t device_name_length,
                                        char *device_unique_id_utf8,
                                        uint32_t device_unique_id_utf8_length,
                                        char *product_unique_id_utf8,
                                        uint32_t product_unique_id_utf8_length) {
  std::lock_guard<std::mutex> lock(api_lock_);
  const int32_t result =
      get_device_info(device_number, device_name_utf8, device_name_length, device_unique_id_utf8,
                      device_unique_id_utf8_length, product_unique_id_utf8,
                      product_unique_id_utf8_length);
  return result > static_cast<int32_t>(device_number) ? 0 : -1;
}

int32_t device_info_ds::get_device_info(uint32_t device_number, char *device_name_utf8,
                                        uint32_t device_name_length,
                                        char *device_unique_id_utf8,
                                        uint32_t device_unique_id_utf8_length,
                                        char *product_unique_id_utf8,
                                        uint32_t product_unique_id_utf8_length) {
  // Enumerate all video capture devices.
  RELEASE_AND_CLEAR(ds_moniker_dev_enum_);
  HRESULT hr = ds_dev_enum_->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                   &ds_moniker_dev_enum_, 0);
  if (hr != NOERROR) {
    LOG_INFO("Failed to enumerate CLSID_SystemDeviceEnum, error 0x{:x}. No webcam exist?",
             static_cast<unsigned long>(hr));
    return 0;
  }

  ds_moniker_dev_enum_->Reset();
  ULONG c_fetched;
  IMoniker *p_m;
  int index = 0;
  while (S_OK == ds_moniker_dev_enum_->Next(1, &p_m, &c_fetched)) {
    IPropertyBag *p_bag;
    hr = p_m->BindToStorage(0, 0, IID_IPropertyBag, (void **)&p_bag);
    if (S_OK == hr) {
      // Find the description or friendly name.
      VARIANT var_name;
      VariantInit(&var_name);
      hr = p_bag->Read(L"Description", &var_name, 0);
      if (FAILED(hr)) {
        hr = p_bag->Read(L"FriendlyName", &var_name, 0);
      }
      if (SUCCEEDED(hr)) {
        // Ignore all VFW drivers and Google Camera Adapter.
        if ((wcsstr(var_name.bstrVal, L"(VFW)") == nullptr) &&
            (_wcsnicmp(var_name.bstrVal, L"Google Camera Adapter", 21) != 0)) {
          // Found a valid device.
          if (index == static_cast<int>(device_number)) {
            int conv_result = 0;
            if (device_name_length > 0) {
              conv_result =
                  WideCharToMultiByte(CP_UTF8, 0, var_name.bstrVal, -1, (char *)device_name_utf8,
                                      device_name_length, nullptr, nullptr);
              if (conv_result == 0) {
                LOG_INFO("Failed to convert device name to UTF8, error = {}", GetLastError());
                return -1;
              }
            }
            if (device_unique_id_utf8_length > 0) {
              hr = p_bag->Read(L"DevicePath", &var_name, 0);
              if (FAILED(hr)) {
                strncpy_s((char *)device_unique_id_utf8, device_unique_id_utf8_length,
                          (char *)device_name_utf8, conv_result);
                LOG_INFO("Failed to get device_unique_id_utf8 using device_name_utf8");
              } else {
                conv_result = WideCharToMultiByte(CP_UTF8, 0, var_name.bstrVal, -1,
                                                  (char *)device_unique_id_utf8,
                                                  device_unique_id_utf8_length, nullptr, nullptr);
                if (conv_result == 0) {
                  LOG_INFO("Failed to convert device name to UTF8, error = {}", GetLastError());
                  return -1;
                }
                if (product_unique_id_utf8 && product_unique_id_utf8_length > 0) {
                  get_product_id(device_unique_id_utf8, product_unique_id_utf8,
                                 product_unique_id_utf8_length);
                }
              }
            }
          }
          ++index; // increase the number of valid devices
        }
      }
      VariantClear(&var_name);
      p_bag->Release();
      p_m->Release();
    }
  }
  if (device_name_length) {
    LOG_INFO("{} {}", __FUNCTION__, device_name_utf8);
  }
  return index;
}

IBaseFilter *device_info_ds::get_device_filter(const char *device_unique_id_utf8,
                                               char *product_unique_id_utf8,
                                               uint32_t product_unique_id_utf8_length) {
  const int32_t device_unique_id_utf8_length =
      static_cast<int32_t>(strlen((char *)device_unique_id_utf8));
  if (device_unique_id_utf8_length >= k_video_capture_unique_name_length) {
    LOG_INFO("Device name too long");
    return nullptr;
  }

  // Enumerate all video capture devices.
  RELEASE_AND_CLEAR(ds_moniker_dev_enum_);
  HRESULT hr = ds_dev_enum_->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                   &ds_moniker_dev_enum_, 0);
  if (hr != NOERROR) {
    LOG_INFO("Failed to enumerate CLSID_SystemDeviceEnum, error 0x{:x}. No webcam exist?",
             static_cast<unsigned long>(hr));
    return nullptr;
  }
  ds_moniker_dev_enum_->Reset();
  ULONG c_fetched;
  IMoniker *p_m;

  IBaseFilter *capture_filter = nullptr;
  bool device_found = false;
  while (S_OK == ds_moniker_dev_enum_->Next(1, &p_m, &c_fetched) && !device_found) {
    IPropertyBag *p_bag;
    hr = p_m->BindToStorage(0, 0, IID_IPropertyBag, (void **)&p_bag);
    if (S_OK == hr) {
      // Find the description or friendly name.
      VARIANT var_name;
      VariantInit(&var_name);
      if (device_unique_id_utf8_length > 0) {
        hr = p_bag->Read(L"DevicePath", &var_name, 0);
        if (FAILED(hr)) {
          hr = p_bag->Read(L"Description", &var_name, 0);
          if (FAILED(hr)) {
            hr = p_bag->Read(L"FriendlyName", &var_name, 0);
          }
        }
        if (SUCCEEDED(hr)) {
          char temp_device_path_utf8[256];
          temp_device_path_utf8[0] = 0;
          WideCharToMultiByte(CP_UTF8, 0, var_name.bstrVal, -1, temp_device_path_utf8,
                              sizeof(temp_device_path_utf8), nullptr, nullptr);
          if (strncmp(temp_device_path_utf8, (const char *)device_unique_id_utf8,
                      device_unique_id_utf8_length) == 0) {
            // We have found the requested device.
            device_found = true;
            hr = p_m->BindToObject(0, 0, IID_IBaseFilter, (void **)&capture_filter);
            if (FAILED(hr)) {
              LOG_ERROR("Failed to bind to the selected capture device 0x{:x}",
                        static_cast<unsigned long>(hr));
            }

            if (product_unique_id_utf8 && product_unique_id_utf8_length > 0) {
              get_product_id(device_unique_id_utf8, product_unique_id_utf8,
                             product_unique_id_utf8_length);
            }
          }
        }
      }
      VariantClear(&var_name);
      p_bag->Release();
    }
    p_m->Release();
  }
  return capture_filter;
}

int32_t device_info_ds::get_windows_capability(
    const int32_t capability_index,
    video_capture_capability_windows &windows_capability) {
  std::lock_guard<std::mutex> lock(api_lock_);

  if (capability_index < 0 ||
      static_cast<size_t>(capability_index) >= capture_capabilities_windows_.size()) {
    return -1;
  }

  windows_capability = capture_capabilities_windows_[capability_index];
  return 0;
}

int32_t device_info_ds::create_capability_map(const char *device_unique_id_utf8) {
  // Reset old capability list.
  capture_capabilities_.clear();
  capture_capabilities_windows_.clear();

  const int32_t device_unique_id_utf8_length =
      static_cast<int32_t>(strlen((char *)device_unique_id_utf8));
  if (device_unique_id_utf8_length >= k_video_capture_unique_name_length) {
    LOG_INFO("Device name too long");
    return -1;
  }
  LOG_INFO("create_capability_map called for device {}", device_unique_id_utf8);

  char product_id[k_video_capture_product_id_length];
  IBaseFilter *capture_device = device_info_ds::get_device_filter(
      device_unique_id_utf8, product_id, k_video_capture_product_id_length);
  if (!capture_device)
    return -1;
  IPin *output_capture_pin = get_output_pin(capture_device, GUID_NULL);
  if (!output_capture_pin) {
    LOG_INFO("Failed to get capture device output pin");
    RELEASE_AND_CLEAR(capture_device);
    return -1;
  }
  IAMExtDevice *ext_device = nullptr;
  HRESULT hr = capture_device->QueryInterface(IID_IAMExtDevice, (void **)&ext_device);
  if (SUCCEEDED(hr) && ext_device) {
    LOG_INFO("This is an external device");
    ext_device->Release();
  }

  IAMStreamConfig *stream_config = nullptr;
  hr = output_capture_pin->QueryInterface(IID_IAMStreamConfig, (void **)&stream_config);
  if (FAILED(hr)) {
    LOG_INFO("Failed to get IID_IAMStreamConfig interface from capture device");
    return -1;
  }

  // This gets the FPS.
  IAMVideoControl *video_control_config = nullptr;
  HRESULT hr_vc =
      capture_device->QueryInterface(IID_IAMVideoControl, (void **)&video_control_config);
  if (FAILED(hr_vc)) {
    LOG_INFO("IID_IAMVideoControl Interface NOT SUPPORTED");
  }

  AM_MEDIA_TYPE *pmt = nullptr;
  VIDEO_STREAM_CONFIG_CAPS caps;
  int count, size;

  hr = stream_config->GetNumberOfCapabilities(&count, &size);
  if (FAILED(hr)) {
    LOG_INFO("Failed to GetNumberOfCapabilities");
    RELEASE_AND_CLEAR(video_control_config);
    RELEASE_AND_CLEAR(stream_config);
    RELEASE_AND_CLEAR(output_capture_pin);
    RELEASE_AND_CLEAR(capture_device);
    return -1;
  }

  // Check if the device supports formattype == FORMAT_VideoInfo2 and
  // FORMAT_VideoInfo. Prefer FORMAT_VideoInfo since some cameras (ZureCam) have
  // been seen having problems with MJPEG and FORMAT_VideoInfo2.
  // Interlace flag is only supported in FORMAT_VideoInfo2.
  bool support_format_video_info2 = false;
  bool support_format_video_info = false;
  bool found_interlaced_format = false;
  GUID preferred_video_format = FORMAT_VideoInfo;
  for (int32_t tmp = 0; tmp < count; ++tmp) {
    hr = stream_config->GetStreamCaps(tmp, &pmt, reinterpret_cast<BYTE *>(&caps));
    if (hr == S_OK) {
      if (pmt->majortype == MEDIATYPE_Video && pmt->formattype == FORMAT_VideoInfo2) {
        LOG_INFO("Device support FORMAT_VideoInfo2");
        support_format_video_info2 = true;
        VIDEOINFOHEADER2 *h = reinterpret_cast<VIDEOINFOHEADER2 *>(pmt->pbFormat);
        assert(h);
        found_interlaced_format |=
            h->dwInterlaceFlags &
            (AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOnly);
      }
      if (pmt->majortype == MEDIATYPE_Video && pmt->formattype == FORMAT_VideoInfo) {
        LOG_INFO("Device support FORMAT_VideoInfo");
        support_format_video_info = true;
      }

      free_media_type(pmt);
      pmt = nullptr;
    }
  }
  if (support_format_video_info2) {
    if (support_format_video_info && !found_interlaced_format) {
      preferred_video_format = FORMAT_VideoInfo;
    } else {
      preferred_video_format = FORMAT_VideoInfo2;
    }
  }

  for (int32_t tmp = 0; tmp < count; ++tmp) {
    hr = stream_config->GetStreamCaps(tmp, &pmt, reinterpret_cast<BYTE *>(&caps));
    if (hr != S_OK) {
      LOG_INFO("Failed to GetStreamCaps");
      RELEASE_AND_CLEAR(video_control_config);
      RELEASE_AND_CLEAR(stream_config);
      RELEASE_AND_CLEAR(output_capture_pin);
      RELEASE_AND_CLEAR(capture_device);
      return -1;
    }

    if (pmt->majortype == MEDIATYPE_Video && pmt->formattype == preferred_video_format) {
      video_capture_capability_windows capability;
      int64_t avg_time_per_frame = 0;

      if (pmt->formattype == FORMAT_VideoInfo) {
        VIDEOINFOHEADER *h = reinterpret_cast<VIDEOINFOHEADER *>(pmt->pbFormat);
        assert(h);
        capability.direct_show_capability_index = tmp;
        capability.width = h->bmiHeader.biWidth;
        capability.height = h->bmiHeader.biHeight;
        avg_time_per_frame = h->AvgTimePerFrame;
      }
      if (pmt->formattype == FORMAT_VideoInfo2) {
        VIDEOINFOHEADER2 *h = reinterpret_cast<VIDEOINFOHEADER2 *>(pmt->pbFormat);
        assert(h);
        capability.direct_show_capability_index = tmp;
        capability.width = h->bmiHeader.biWidth;
        capability.height = h->bmiHeader.biHeight;
        capability.interlaced =
            h->dwInterlaceFlags &
            (AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOnly);
        avg_time_per_frame = h->AvgTimePerFrame;
      }

      if (hr_vc == S_OK) {
        LONGLONG *frame_duration_list = nullptr;
        LONGLONG max_fps = 0;
        long list_size = 0;
        SIZE requested_size;
        requested_size.cx = capability.width;
        requested_size.cy = capability.height;

        // GetMaxAvailableFrameRate doesn't return max frame rate always
        // e.g.: Logitech Notebook. This may be due to a bug in that API
        // because GetFrameRateList array is reversed in the above camera. So
        // a util method written. Can't assume the first value will return
        // the max fps.
        hr_vc = video_control_config->GetFrameRateList(output_capture_pin, tmp, requested_size,
                                                       &list_size, &frame_duration_list);

        if (hr_vc == S_OK) {
          max_fps = get_max_of_frame_array(frame_duration_list, list_size);
        }

        CoTaskMemFree(frame_duration_list);
        frame_duration_list = nullptr;
        list_size = 0;

        // On some odd cameras, you may get a 0 for duration. Some others may
        // not update the out vars. GetMaxOfFrameArray returns the lowest
        // duration (highest FPS), or 0 if there was no list with elements.
        if (0 != max_fps) {
          capability.max_fps = static_cast<int>(10000000 / max_fps);
          capability.support_frame_rate_control = true;
        } else {
          LOG_INFO("GetMaxAvailableFrameRate NOT SUPPORTED");
          if (avg_time_per_frame > 0)
            capability.max_fps = static_cast<int>(10000000 / avg_time_per_frame);
          else
            capability.max_fps = 0;
        }
      } else {
        if (avg_time_per_frame > 0)
          capability.max_fps = static_cast<int>(10000000 / avg_time_per_frame);
        else
          capability.max_fps = 0;
      }

      // Map video subtypes.
      if (pmt->subtype == MEDIASUBTYPE_I420) {
        capability.video_type = video_type::k_i420;
      } else if (pmt->subtype == MEDIASUBTYPE_IYUV) {
        capability.video_type = video_type::k_iyuv;
      } else if (pmt->subtype == MEDIASUBTYPE_RGB24) {
        capability.video_type = video_type::k_rgb24;
      } else if (pmt->subtype == MEDIASUBTYPE_YUY2) {
        capability.video_type = video_type::k_yuy2;
      } else if (pmt->subtype == MEDIASUBTYPE_RGB565) {
        capability.video_type = video_type::k_rgb565;
      } else if (pmt->subtype == MEDIASUBTYPE_MJPG) {
        capability.video_type = video_type::k_mjpeg;
      } else if (pmt->subtype == MEDIASUBTYPE_dvsl || pmt->subtype == MEDIASUBTYPE_dvsd ||
                 pmt->subtype == MEDIASUBTYPE_dvhd) {
        // External DV camera — MS DV filter seems to create YUY2 type.
        capability.video_type = video_type::k_yuy2;
      } else if (pmt->subtype == MEDIASUBTYPE_UYVY) {
        capability.video_type = video_type::k_uyvy;
      } else if (pmt->subtype == MEDIASUBTYPE_HDYC) {
        LOG_INFO("Device support HDYC.");
        capability.video_type = video_type::k_uyvy;
      } else {
        WCHAR str_guid[39];
        StringFromGUID2(pmt->subtype, str_guid, 39);
        char guid_utf8[80];
        WideCharToMultiByte(CP_UTF8, 0, str_guid, -1, guid_utf8, sizeof(guid_utf8), nullptr,
                            nullptr);
        LOG_WARN("Device support unknown media type {}, width {}, height {}", guid_utf8,
                 capability.width, capability.height);
        free_media_type(pmt);
        pmt = nullptr;
        continue;
      }

      capture_capabilities_.push_back(capability);
      capture_capabilities_windows_.push_back(capability);
      LOG_INFO("Camera capability, width:{} height:{} type:{} fps:{}", capability.width,
               capability.height, static_cast<int>(capability.video_type), capability.max_fps);
    }
    free_media_type(pmt);
    pmt = nullptr;
  }
  RELEASE_AND_CLEAR(stream_config);
  RELEASE_AND_CLEAR(video_control_config);
  RELEASE_AND_CLEAR(output_capture_pin);
  RELEASE_AND_CLEAR(capture_device);

  // Store the new used device name.
  last_used_device_name_length_ = device_unique_id_utf8_length;
  last_used_device_name_ =
      (char *)realloc(last_used_device_name_, last_used_device_name_length_ + 1);
  memcpy(last_used_device_name_, device_unique_id_utf8, last_used_device_name_length_ + 1);
  LOG_INFO("create_capability_map {}", capture_capabilities_.size());

  return static_cast<int32_t>(capture_capabilities_.size());
}

// Constructs a product ID from the Windows DevicePath. On a USB device the
// devicePath contains product id and vendor id. This seems to work for firewire
// as well.
// Example of device path:
// "\\?\usb#vid_0408&pid_2010&mi_00#7&258e7aaf&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global"
// "\\?\avc#sony&dv-vcr&camcorder&dv#65b2d50301460008#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global"
void device_info_ds::get_product_id(const char *device_path, char *product_unique_id_utf8,
                                    uint32_t product_unique_id_utf8_length) {
  *product_unique_id_utf8 = '\0';
  char *start_pos = strstr((char *)device_path, "\\\\?\\");
  if (!start_pos) {
    strncpy_s((char *)product_unique_id_utf8, product_unique_id_utf8_length, "", 1);
    LOG_INFO("Failed to get the product Id");
    return;
  }
  start_pos += 4;

  char *pos = strchr(start_pos, '&');
  if (!pos || pos >= (char *)device_path + strlen((char *)device_path)) {
    strncpy_s((char *)product_unique_id_utf8, product_unique_id_utf8_length, "", 1);
    LOG_INFO("Failed to get the product Id");
    return;
  }
  // Find the second occurrence.
  pos = strchr(pos + 1, '&');
  uint32_t bytes_to_copy = static_cast<uint32_t>(pos - start_pos);
  if (pos && (bytes_to_copy < product_unique_id_utf8_length) &&
      bytes_to_copy <= k_video_capture_product_id_length) {
    strncpy_s((char *)product_unique_id_utf8, product_unique_id_utf8_length, (char *)start_pos,
              bytes_to_copy);
  } else {
    strncpy_s((char *)product_unique_id_utf8, product_unique_id_utf8_length, "", 1);
    LOG_INFO("Failed to get the product Id");
  }
}

} // namespace base
} // namespace traa
