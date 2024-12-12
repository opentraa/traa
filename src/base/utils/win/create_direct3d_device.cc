/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/utils/win/create_direct3d_device.h"

#include <libloaderapi.h>

#include <utility>

namespace {

FARPROC load_d3d11_function(const char *function_name) {
  static HMODULE const handle =
      ::LoadLibraryExW(L"d3d11.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  return handle ? ::GetProcAddress(handle, function_name) : nullptr;
}

decltype(&::CreateDirect3D11DeviceFromDXGIDevice)
get_create_direct3d11_device_from_dxgi_device_function() {
  static decltype(&::CreateDirect3D11DeviceFromDXGIDevice) const function =
      reinterpret_cast<decltype(&::CreateDirect3D11DeviceFromDXGIDevice)>(
          load_d3d11_function("CreateDirect3D11DeviceFromDXGIDevice"));
  return function;
}

} // namespace

namespace traa {
namespace base {

bool resolve_core_winrt_direct3d_delayload() {
  return get_create_direct3d11_device_from_dxgi_device_function();
}

HRESULT create_direct3d_device_from_dxgi_device(
    IDXGIDevice *dxgi_device,
    ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice **out_d3d11_device) {
  decltype(&::CreateDirect3D11DeviceFromDXGIDevice) create_d3d11_device_func =
      get_create_direct3d11_device_from_dxgi_device_function();
  if (!create_d3d11_device_func)
    return E_FAIL;

  Microsoft::WRL::ComPtr<IInspectable> inspectable_surface;
  HRESULT hr = create_d3d11_device_func(dxgi_device, &inspectable_surface);
  if (FAILED(hr))
    return hr;

  return inspectable_surface->QueryInterface(IID_PPV_ARGS(out_d3d11_device));
}

} // namespace base
} // namespace traa