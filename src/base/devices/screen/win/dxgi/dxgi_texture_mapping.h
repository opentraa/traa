/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_MAPPING_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_MAPPING_H_

#include <d3d11.h>
#include <dxgi1_2.h>

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/win/dxgi/dxgi_texture.h"

namespace traa {
namespace base {

// A dxgi_texture which directly maps bitmap from IDXGIResource. This class is
// used when DXGI_OUTDUPL_DESC.DesktopImageInSystemMemory is true. (This usually
// means the video card shares main memory with CPU, instead of having its own
// individual memory.)
class dxgi_texture_mapping : public dxgi_texture {
public:
  // Creates a dxgi_texture_mapping instance. Caller must maintain the lifetime
  // of input `duplication` to make sure it outlives this instance.
  explicit dxgi_texture_mapping(IDXGIOutputDuplication *duplication);

  ~dxgi_texture_mapping() override;

protected:
  bool copy_from_texture(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                         ID3D11Texture2D *texture) override;

  bool do_release() override;

private:
  IDXGIOutputDuplication *const duplication_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_MAPPING_H_
