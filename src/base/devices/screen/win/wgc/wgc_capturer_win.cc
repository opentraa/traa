/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_capturer_win.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/win/wgc/wgc_desktop_frame.h"
#include "base/logger.h"
#include "base/utils/time_utils.h"
#include "base/utils/win/get_activation_factory.h"
#include "base/utils/win/hstring.h"
#include "base/utils/win/version.h"

#include <DispatcherQueue.h>
#include <windows.foundation.metadata.h>
#include <windows.graphics.capture.h>

#include <utility>

namespace WGC = ABI::Windows::Graphics::Capture;
using Microsoft::WRL::ComPtr;

namespace traa {
namespace base {

inline namespace {

constexpr wchar_t k_core_messaging_dll[] = L"CoreMessaging.dll";

constexpr wchar_t k_wgc_session_type[] = L"Windows.Graphics.Capture.GraphicsCaptureSession";
constexpr wchar_t k_api_contract[] = L"Windows.Foundation.UniversalApiContract";
constexpr wchar_t k_dirty_region_mode[] = L"DirtyRegionMode";
constexpr UINT16 k_required_api_contract_version = 8;

enum class wgc_capturer_result {
  success = 0,
  no_direct_3d_device = 1,
  no_source_selected = 2,
  item_creation_failure = 3,
  session_start_failure = 4,
  get_frame_failure = 5,
  frame_dropped = 6,
  create_dispatcher_queue_failure = 7,
  max_value = create_dispatcher_queue_failure
};

void record_wgc_capturer_result(wgc_capturer_result error) {
  LOG_EVENT("SDM", "wgc_capturer_result", static_cast<int>(error));
}

// Checks if the DirtyRegionMode property is present in GraphicsCaptureSession
// and logs a boolean histogram with the result.
// TODO(https://crbug.com/40259177): Detecting support for this property means
// that the WGC API supports dirty regions and it can be utilized to improve
// the capture performance and the existing zero-herz support.
void log_dirty_region_support() {
  ComPtr<ABI::Windows::Foundation::Metadata::IApiInformationStatics> api_info_statics;
  HRESULT hr = get_activation_factory<ABI::Windows::Foundation::Metadata::IApiInformationStatics,
                                      RuntimeClass_Windows_Foundation_Metadata_ApiInformation>(
      &api_info_statics);
  if (FAILED(hr)) {
    return;
  }

  HSTRING dirty_region_mode;
  hr = create_hstring(k_dirty_region_mode, static_cast<uint32_t>(wcslen(k_dirty_region_mode)),
                      &dirty_region_mode);
  if (FAILED(hr)) {
    delete_hstring(dirty_region_mode);
    return;
  }

  HSTRING wgc_session_type;
  hr = create_hstring(k_wgc_session_type, static_cast<uint32_t>(wcslen(k_wgc_session_type)),
                      &wgc_session_type);
  if (SUCCEEDED(hr)) {
    boolean is_dirty_region_mode_supported = false;
    api_info_statics->IsPropertyPresent(wgc_session_type, dirty_region_mode,
                                        &is_dirty_region_mode_supported);
    LOG_EVENT_COND("SDM", "wgc_dirty_region_support", !!is_dirty_region_mode_supported);
  }

  delete_hstring(dirty_region_mode);
  delete_hstring(wgc_session_type);
}

} // namespace

wgc_capturer_win::wgc_capturer_win(const desktop_capture_options &options,
                                   std::unique_ptr<wgc_capture_source_factory> source_factory,
                                   std::unique_ptr<source_enumerator> enumerator,
                                   bool allow_delayed_capturable_check)
    : options_(options), source_factory_(std::move(source_factory)),
      source_enumerator_(std::move(enumerator)),
      allow_delayed_capturable_check_(allow_delayed_capturable_check) {
  if (!core_messaging_library_)
    core_messaging_library_ = ::LoadLibraryW(k_core_messaging_dll);

  if (core_messaging_library_) {
    create_dispatcher_queue_controller_func_ =
        reinterpret_cast<CreateDispatcherQueueControllerFunc>(
            ::GetProcAddress(core_messaging_library_, "CreateDispatcherQueueController"));
  }
  log_dirty_region_support();
}

wgc_capturer_win::~wgc_capturer_win() {
  if (core_messaging_library_)
    ::FreeLibrary(core_messaging_library_);
}

// static
bool wgc_capturer_win::is_wgc_supported(capture_type capture_type) {
  if (!capture_utils::has_active_display()) {
    // There is a bug in `CreateForMonitor` that causes a crash if there are no
    // active displays. The crash was fixed in Win11, but we are still unable
    // to capture screens without an active display.
    if (capture_type == capture_type::screen)
      return false;

    // There is a bug in the DWM (Desktop Window Manager) that prevents it from
    // providing image data if there are no displays attached. This was fixed in
    // Windows 11.
    if (os_get_version() < version_alias::VERSION_WIN11)
      return false;
  }

  // A bug in the WGC API `CreateForMonitor` prevents capturing the entire
  // virtual screen (all monitors simultaneously), this was fixed in 20H1. Since
  // we can't assert that we won't be asked to capture the entire virtual
  // screen, we report unsupported so we can fallback to another capturer.
  if (capture_type == capture_type::screen &&
      os_get_version() < version_alias::VERSION_WIN10_20H1) {
    return false;
  }

  if (!resolve_core_winrt_delayload())
    return false;

  // We need to check if the WGC APIs are present on the system. Certain SKUs
  // of Windows ship without these APIs.
  ComPtr<ABI::Windows::Foundation::Metadata::IApiInformationStatics> api_info_statics;
  HRESULT hr = get_activation_factory<ABI::Windows::Foundation::Metadata::IApiInformationStatics,
                                      RuntimeClass_Windows_Foundation_Metadata_ApiInformation>(
      &api_info_statics);
  if (FAILED(hr))
    return false;

  HSTRING api_contract;
  hr = create_hstring(k_api_contract, static_cast<uint32_t>(wcslen(k_api_contract)), &api_contract);
  if (FAILED(hr))
    return false;

  boolean is_api_present;
  hr = api_info_statics->IsApiContractPresentByMajor(api_contract, k_required_api_contract_version,
                                                     &is_api_present);
  delete_hstring(api_contract);
  if (FAILED(hr) || !is_api_present)
    return false;

  HSTRING wgc_session_type;
  hr = create_hstring(k_wgc_session_type, static_cast<uint32_t>(wcslen(k_wgc_session_type)),
                      &wgc_session_type);
  if (FAILED(hr))
    return false;

  boolean is_type_present;
  hr = api_info_statics->IsTypePresent(wgc_session_type, &is_type_present);
  delete_hstring(wgc_session_type);
  if (FAILED(hr) || !is_type_present)
    return false;

  // If the APIs are present, we need to check that they are supported.
  ComPtr<WGC::IGraphicsCaptureSessionStatics> capture_session_statics;
  hr = get_activation_factory<WGC::IGraphicsCaptureSessionStatics,
                              RuntimeClass_Windows_Graphics_Capture_GraphicsCaptureSession>(
      &capture_session_statics);
  if (FAILED(hr))
    return false;

  boolean is_supported;
  hr = capture_session_statics->IsSupported(&is_supported);
  if (FAILED(hr) || !is_supported)
    return false;

  return true;
}

std::unique_ptr<desktop_capturer>
wgc_capturer_win::create_raw_window_capturer(const desktop_capture_options &options,
                                             bool allow_delayed_capturable_check) {
  return std::make_unique<wgc_capturer_win>(
      options, std::make_unique<wgc_window_source_factory>(),
      std::make_unique<window_enumerator>(options.enumerate_current_process_windows()),
      allow_delayed_capturable_check);
}

// static
std::unique_ptr<desktop_capturer>
wgc_capturer_win::create_raw_screen_capturer(const desktop_capture_options &options) {
  return std::make_unique<wgc_capturer_win>(options, std::make_unique<wgc_screen_source_factory>(),
                                            std::make_unique<screen_enumerator>(), false);
}

bool wgc_capturer_win::get_source_list(desktop_capturer::source_list_t *sources) {
  return source_enumerator_->find_all_sources(sources);
}

bool wgc_capturer_win::select_source(desktop_capturer::source_id_t id) {
  capture_source_ = source_factory_->create_capture_source(id);
  if (allow_delayed_capturable_check_)
    return true;

  return capture_source_->is_capturable();
}

bool wgc_capturer_win::focus_on_selected_source() {
  if (!capture_source_)
    return false;

  return capture_source_->focus_on_source();
}

void wgc_capturer_win::start(desktop_capturer::capture_callback *callback) {
  LOG_EVENT("SDM", "wgc_capturer_win::start", static_cast<int>(current_capturer_id()));

  callback_ = callback;

  // Create a Direct3D11 device to share amongst the WgcCaptureSessions. Many
  // parameters are nullptr as the implemention uses defaults that work well for
  // us.
  HRESULT hr = ::D3D11CreateDevice(
      /*adapter=*/nullptr, D3D_DRIVER_TYPE_HARDWARE,
      /*software_rasterizer=*/nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
      /*feature_levels=*/nullptr, /*feature_levels_size=*/0, D3D11_SDK_VERSION, &d3d11_device_,
      /*feature_level=*/nullptr, /*device_context=*/nullptr);
  if (hr == DXGI_ERROR_UNSUPPORTED) {
    // If a hardware device could not be created, use WARP which is a high speed
    // software device.
    hr = ::D3D11CreateDevice(
        /*adapter=*/nullptr, D3D_DRIVER_TYPE_WARP,
        /*software_rasterizer=*/nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        /*feature_levels=*/nullptr, /*feature_levels_size=*/0, D3D11_SDK_VERSION, &d3d11_device_,
        /*feature_level=*/nullptr,
        /*device_context=*/nullptr);
  }

  if (FAILED(hr)) {
    LOG_ERROR("failed to create D3D11Device: {}", hr);
  }
}

void wgc_capturer_win::capture_frame() {
  if (!capture_source_) {
    LOG_ERROR("source hasn't been selected");
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                 /*frame=*/nullptr);
    record_wgc_capturer_result(wgc_capturer_result::no_source_selected);
    return;
  }

  if (!d3d11_device_) {
    LOG_ERROR("no D3D11D3evice, cannot capture.");
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                 /*frame=*/nullptr);
    record_wgc_capturer_result(wgc_capturer_result::no_direct_3d_device);
    return;
  }

  if (allow_delayed_capturable_check_ && !capture_source_->is_capturable()) {
    LOG_ERROR("source is not capturable.");
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                 /*frame=*/nullptr);
    return;
  }

  HRESULT hr;
  if (!dispatcher_queue_created_) {
    // Set the apartment type to NONE because this thread should already be COM
    // initialized.
    DispatcherQueueOptions options{sizeof(DispatcherQueueOptions),
                                   DISPATCHERQUEUE_THREAD_TYPE::DQTYPE_THREAD_CURRENT,
                                   DISPATCHERQUEUE_THREAD_APARTMENTTYPE::DQTAT_COM_NONE};
    ComPtr<ABI::Windows::System::IDispatcherQueueController> queue_controller;
    hr = create_dispatcher_queue_controller_func_(options, &queue_controller);

    // If there is already a DispatcherQueue on this thread, that is fine. Its
    // lifetime is tied to the thread's, and as long as the thread has one, even
    // if we didn't create it, the capture session's events will be delivered on
    // this thread.
    if (FAILED(hr) && hr != RPC_E_WRONG_THREAD) {
      record_wgc_capturer_result(wgc_capturer_result::create_dispatcher_queue_failure);
      callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                   /*frame=*/nullptr);
    } else {
      dispatcher_queue_created_ = true;
    }
  }

  int64_t capture_start_time_nanos = time_nanos();

  wgc_capture_session *capture_session = nullptr;
  std::map<desktop_capturer::source_id_t, wgc_capture_session>::iterator session_iter =
      ongoing_captures_.find(capture_source_->get_source_id());
  if (session_iter == ongoing_captures_.end()) {
    ComPtr<WGC::IGraphicsCaptureItem> item;
    hr = capture_source_->get_capture_item(&item);
    if (FAILED(hr)) {
      LOG_ERROR("failed to create a GraphicsCaptureItem: {}", hr);
      callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                   /*frame=*/nullptr);
      record_wgc_capturer_result(wgc_capturer_result::item_creation_failure);
      return;
    }

    std::pair<std::map<desktop_capturer::source_id_t, wgc_capture_session>::iterator, bool>
        iter_success_pair = ongoing_captures_.emplace(
            std::piecewise_construct, std::forward_as_tuple(capture_source_->get_source_id()),
            std::forward_as_tuple(d3d11_device_, item, capture_source_->get_size()));
    capture_session = &iter_success_pair.first->second;
  } else {
    capture_session = &session_iter->second;
  }

  if (!capture_session->is_capture_started()) {
    hr = capture_session->start_capture(options_);
    if (FAILED(hr)) {
      LOG_ERROR("failed to start capture: {}", hr);
      ongoing_captures_.erase(capture_source_->get_source_id());
      callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                   /*frame=*/nullptr);
      record_wgc_capturer_result(wgc_capturer_result::session_start_failure);
      return;
    }
  }

  std::unique_ptr<desktop_frame> frame;
  if (!capture_session->get_frame(&frame, capture_source_->is_capturable())) {
    LOG_ERROR("GetFrame failed.");
    ongoing_captures_.erase(capture_source_->get_source_id());
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent,
                                 /*frame=*/nullptr);
    record_wgc_capturer_result(wgc_capturer_result::get_frame_failure);
    return;
  }

  if (!frame) {
    callback_->on_capture_result(desktop_capturer::capture_result::error_temporary,
                                 /*frame=*/nullptr);
    record_wgc_capturer_result(wgc_capturer_result::frame_dropped);
    return;
  }

  int64_t capture_time_ms = (time_nanos() - capture_start_time_nanos) / k_num_nanosecs_per_millisec;
  frame->set_capture_time_ms(capture_time_ms);
  frame->set_capturer_id(desktop_capture_id::k_capture_wgc);
  frame->set_may_contain_cursor(options_.prefer_cursor_embedded());
  frame->set_top_left(capture_source_->get_top_left());
  record_wgc_capturer_result(wgc_capturer_result::success);
  callback_->on_capture_result(desktop_capturer::capture_result::success, std::move(frame));
}

bool wgc_capturer_win::is_source_being_captured(desktop_capturer::source_id_t id) {
  std::map<desktop_capturer::source_id_t, wgc_capture_session>::iterator session_iter =
      ongoing_captures_.find(id);
  if (session_iter == ongoing_captures_.end())
    return false;

  return session_iter->second.is_capture_started();
}

} // namespace base
} // namespace traa
