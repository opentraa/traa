/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_D3D_DEVICE_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_D3D_DEVICE_H_

#include <comdef.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <vector>

namespace traa {
namespace base {

// A wrapper of ID3D11Device and its corresponding context and IDXGIAdapter.
// This class represents one video card in the system.
class d3d_device {
public:
  d3d_device(const d3d_device &other);
  d3d_device(d3d_device &&other);
  ~d3d_device();

  ID3D11Device *get_d3d_device() const { return d3d_device_.Get(); }

  ID3D11DeviceContext *context() const { return context_.Get(); }

  IDXGIDevice *dxgi_device() const { return dxgi_device_.Get(); }

  IDXGIAdapter *dxgi_adapter() const { return dxgi_adapter_.Get(); }

  // Returns all d3d_device instances on the system. Returns an empty vector if
  // anything wrong.
  static std::vector<d3d_device> enum_devices();

private:
  // Instances of d3d_device should only be created by enum_devices() static
  // function.
  d3d_device();

  // Initializes the d3d_device from an IDXGIAdapter.
  bool initialize(const Microsoft::WRL::ComPtr<IDXGIAdapter> &adapter);

  Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device_;
  Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_D3D_DEVICE_H_
