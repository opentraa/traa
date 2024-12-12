/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_STAGING_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_STAGING_H_

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/win/d3d_device.h"
#include "base/devices/screen/win/dxgi/dxgi_texture.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace traa {
namespace base {

// A pair of an ID3D11Texture2D and an IDXGISurface. We need an ID3D11Texture2D
// instance to copy GPU texture to RAM, but an IDXGISurface instance to map the
// texture into a bitmap buffer. These two instances are pointing to a same
// object.
//
// An ID3D11Texture2D is created by an ID3D11Device, so a dxgi_texture cannot be
// shared between two dxgi_adapter_duplicators.
class dxgi_texture_staging : public dxgi_texture {
public:
  // Creates a dxgi_texture_staging instance. Caller must maintain the lifetime
  // of input device to make sure it outlives this instance.
  explicit dxgi_texture_staging(const d3d_device &device);

  ~dxgi_texture_staging() override;

protected:
  // Copies selected regions of a frame represented by frame_info and texture.
  // Returns false if anything wrong.
  bool copy_from_texture(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                         ID3D11Texture2D *texture) override;

  bool do_release() override;

private:
  // Initializes stage_ from a CPU inaccessible IDXGIResource. Returns false if
  // it failed to execute Windows APIs, or the size of the texture is not
  // consistent with desktop_rect.
  bool initialize_stage(ID3D11Texture2D *texture);

  // Makes sure stage_ and surface_ are always pointing to a same object.
  // We need an ID3D11Texture2D instance for
  // ID3D11DeviceContext::CopySubresourceRegion, but an IDXGISurface for
  // IDXGISurface::Map.
  void assert_stage_and_surface_are_same_object();

  const desktop_rect desktop_rect_;
  const d3d_device device_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> stage_;
  Microsoft::WRL::ComPtr<IDXGISurface> surface_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_STAGING_H_
