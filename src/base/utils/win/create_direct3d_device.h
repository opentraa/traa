/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_UTILS_WIN_CREATE_DIRECT3D_DEVICE_H_
#define TRAA_BASE_UTILS_WIN_CREATE_DIRECT3D_DEVICE_H_

#include <windows.graphics.directX.direct3d11.h>
#include <windows.graphics.directX.direct3d11.interop.h>
#include <winerror.h>
#include <wrl/client.h>

namespace traa {
namespace base {

// Callers must check the return value of resolve_core_winrt_direct3d_delayload()
// before using create_direct3d_device_from_dxgi_device().
bool resolve_core_winrt_direct3d_delayload();

// Allows for the creating of Direct3D Devices from a DXGI device on versions
// of Windows greater than Win7.
HRESULT create_direct3d_device_from_dxgi_device(
    IDXGIDevice *dxgi_device,
    ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice **out_d3d11_device);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_UTILS_WIN_CREATE_DIRECT3D_DEVICE_H_