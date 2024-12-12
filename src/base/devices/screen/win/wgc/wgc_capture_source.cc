/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_capture_source.h"

#include "base/devices/screen/win/capture_utils.h"
#include "base/utils/win/get_activation_factory.h"

#include <dwmapi.h>
#include <windows.graphics.capture.interop.h>
#include <windows.h>

#include <utility>

using Microsoft::WRL::ComPtr;
namespace WGC = ABI::Windows::Graphics::Capture;

namespace traa {
namespace base {

wgc_capture_source::wgc_capture_source(desktop_capturer::source_id_t source_id)
    : source_id_(source_id) {}
wgc_capture_source::~wgc_capture_source() = default;

bool wgc_capture_source::should_be_capturable() { return true; }

bool wgc_capture_source::is_capturable() {
  // If we can create a capture item, then we can capture it. Unfortunately,
  // we can't cache this item because it may be created in a different COM
  // apartment than where capture will eventually start from.
  ComPtr<WGC::IGraphicsCaptureItem> item;
  return SUCCEEDED(create_capture_item(&item));
}

bool wgc_capture_source::focus_on_source() { return false; }

ABI::Windows::Graphics::SizeInt32 wgc_capture_source::get_size() {
  if (!item_)
    return {0, 0};

  ABI::Windows::Graphics::SizeInt32 item_size;
  HRESULT hr = item_->get_Size(&item_size);
  if (FAILED(hr))
    return {0, 0};

  return item_size;
}

HRESULT wgc_capture_source::get_capture_item(ComPtr<WGC::IGraphicsCaptureItem> *result) {
  HRESULT hr = S_OK;
  if (!item_)
    hr = create_capture_item(&item_);

  *result = item_;
  return hr;
}

wgc_capture_source_factory::~wgc_capture_source_factory() = default;

wgc_window_source_factory::wgc_window_source_factory() = default;
wgc_window_source_factory::~wgc_window_source_factory() = default;

std::unique_ptr<wgc_capture_source>
wgc_window_source_factory::create_capture_source(desktop_capturer::source_id_t source_id) {
  return std::make_unique<wgc_window_source>(source_id);
}

wgc_screen_source_factory::wgc_screen_source_factory() = default;
wgc_screen_source_factory::~wgc_screen_source_factory() = default;

std::unique_ptr<wgc_capture_source>
wgc_screen_source_factory::create_capture_source(desktop_capturer::source_id_t source_id) {
  return std::make_unique<wgc_screen_source>(source_id);
}

wgc_window_source::wgc_window_source(desktop_capturer::source_id_t source_id)
    : wgc_capture_source(source_id) {}
wgc_window_source::~wgc_window_source() = default;

desktop_vector wgc_window_source::get_top_left() {
  desktop_rect window_rect;
  if (!capture_utils::get_window_rect(reinterpret_cast<HWND>(get_source_id()), &window_rect))
    return desktop_vector();

  return window_rect.top_left();
}

ABI::Windows::Graphics::SizeInt32 wgc_window_source::get_size() {
  RECT window_rect;
  HRESULT hr =
      ::DwmGetWindowAttribute(reinterpret_cast<HWND>(get_source_id()), DWMWA_EXTENDED_FRAME_BOUNDS,
                              reinterpret_cast<void *>(&window_rect), sizeof(window_rect));
  if (FAILED(hr))
    return wgc_capture_source::get_size();

  return {window_rect.right - window_rect.left, window_rect.bottom - window_rect.top};
}

bool wgc_window_source::should_be_capturable() {
  return capture_utils::is_window_valid_and_visible(reinterpret_cast<HWND>(get_source_id()));
}

bool wgc_window_source::is_capturable() {
  if (!should_be_capturable()) {
    return false;
  }

  return wgc_capture_source::is_capturable();
}

bool wgc_window_source::focus_on_source() {
  if (!capture_utils::is_window_valid_and_visible(reinterpret_cast<HWND>(get_source_id())))
    return false;

  return ::BringWindowToTop(reinterpret_cast<HWND>(get_source_id())) &&
         ::SetForegroundWindow(reinterpret_cast<HWND>(get_source_id()));
}

HRESULT wgc_window_source::create_capture_item(ComPtr<WGC::IGraphicsCaptureItem> *result) {
  if (!resolve_core_winrt_delayload())
    return E_FAIL;

  ComPtr<IGraphicsCaptureItemInterop> interop;
  HRESULT hr =
      get_activation_factory<IGraphicsCaptureItemInterop,
                             RuntimeClass_Windows_Graphics_Capture_GraphicsCaptureItem>(&interop);
  if (FAILED(hr))
    return hr;

  ComPtr<WGC::IGraphicsCaptureItem> item;
  hr = interop->CreateForWindow(reinterpret_cast<HWND>(get_source_id()), IID_PPV_ARGS(&item));
  if (FAILED(hr))
    return hr;

  if (!item)
    return E_HANDLE;

  *result = std::move(item);
  return hr;
}

wgc_screen_source::wgc_screen_source(desktop_capturer::source_id_t source_id)
    : wgc_capture_source(source_id) {
  // Getting the HMONITOR could fail if the source_id is invalid. In that case,
  // we leave hmonitor_ uninitialized and `is_capturable()` will fail.
  HMONITOR hmon;
  if (capture_utils::get_hmonitor_from_device_index(get_source_id(), &hmon))
    hmonitor_ = hmon;
}

wgc_screen_source::~wgc_screen_source() = default;

desktop_vector wgc_screen_source::get_top_left() {
  if (!hmonitor_)
    return desktop_vector();

  return capture_utils::get_monitor_rect(*hmonitor_).top_left();
}

ABI::Windows::Graphics::SizeInt32 wgc_screen_source::get_size() {
  ABI::Windows::Graphics::SizeInt32 size = wgc_capture_source::get_size();
  if (!hmonitor_ || (size.Width != 0 && size.Height != 0))
    return size;

  desktop_rect rect = capture_utils::get_monitor_rect(*hmonitor_);
  return {rect.width(), rect.height()};
}

bool wgc_screen_source::is_capturable() {
  if (!hmonitor_)
    return false;

  if (!capture_utils::is_monitor_valid(*hmonitor_))
    return false;

  return wgc_capture_source::is_capturable();
}

HRESULT wgc_screen_source::create_capture_item(ComPtr<WGC::IGraphicsCaptureItem> *result) {
  if (!hmonitor_)
    return E_ABORT;

  if (!resolve_core_winrt_delayload())
    return E_FAIL;

  ComPtr<IGraphicsCaptureItemInterop> interop;
  HRESULT hr =
      get_activation_factory<IGraphicsCaptureItemInterop,
                             RuntimeClass_Windows_Graphics_Capture_GraphicsCaptureItem>(&interop);
  if (FAILED(hr))
    return hr;

  // Ensure the monitor is still valid (hasn't disconnected) before trying to
  // create the item. On versions of Windows before Win11, `CreateForMonitor`
  // will crash if no displays are connected.
  if (!capture_utils::is_monitor_valid(hmonitor_.value()))
    return E_ABORT;

  ComPtr<WGC::IGraphicsCaptureItem> item;
  hr = interop->CreateForMonitor(*hmonitor_, IID_PPV_ARGS(&item));
  if (FAILED(hr))
    return hr;

  if (!item)
    return E_HANDLE;

  *result = std::move(item);
  return hr;
}

} // namespace base
} // namespace traa
