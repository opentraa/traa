/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_texture.h"

#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/win/desktop_capture_utils.h"
#include "base/logger.h"

#include <comdef.h>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace traa {
namespace base {

namespace {

class dxgi_desktop_frame : public desktop_frame {
public:
  explicit dxgi_desktop_frame(const dxgi_texture &texture)
      : desktop_frame(texture.get_desktop_size(), texture.pitch(), texture.bits(), nullptr) {}

  ~dxgi_desktop_frame() override = default;
};

} // namespace

dxgi_texture::dxgi_texture() = default;
dxgi_texture::~dxgi_texture() = default;

bool dxgi_texture::copy_from(const DXGI_OUTDUPL_FRAME_INFO &frame_info, IDXGIResource *resource) {
  ComPtr<ID3D11Texture2D> texture;
  _com_error error = resource->QueryInterface(__uuidof(ID3D11Texture2D),
                                              reinterpret_cast<void **>(texture.GetAddressOf()));
  if (error.Error() != S_OK || !texture) {
    LOG_ERROR("failed to convert IDXGIResource to ID3D11Texture2D: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  D3D11_TEXTURE2D_DESC desc = {0};
  texture->GetDesc(&desc);
  desktop_size_.set(desc.Width, desc.Height);

  return copy_from_texture(frame_info, texture.Get());
}

const desktop_frame &dxgi_texture::as_desktop_frame() {
  if (!frame_) {
    frame_.reset(new dxgi_desktop_frame(*this));
  }
  return *frame_;
}

bool dxgi_texture::release() {
  frame_.reset();
  return do_release();
}

DXGI_MAPPED_RECT *dxgi_texture::rect() { return &rect_; }

} // namespace base
} // namespace traa
