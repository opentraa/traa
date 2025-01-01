/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_texture_staging.h"

#include "base/checks.h"
#include "base/devices/screen/win/desktop_capture_utils.h"
#include "base/logger.h"

#include <comdef.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <unknwn.h>

using Microsoft::WRL::ComPtr;

namespace traa {
namespace base {

dxgi_texture_staging::dxgi_texture_staging(const d3d_device &device) : device_(device) {}

dxgi_texture_staging::~dxgi_texture_staging() = default;

bool dxgi_texture_staging::initialize_stage(ID3D11Texture2D *texture) {
  TRAA_DCHECK(texture);
  D3D11_TEXTURE2D_DESC desc = {0};
  texture->GetDesc(&desc);

  desc.ArraySize = 1;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.MipLevels = 1;
  desc.MiscFlags = 0;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_STAGING;
  if (stage_) {
    assert_stage_and_surface_are_same_object();
    D3D11_TEXTURE2D_DESC current_desc;
    stage_->GetDesc(&current_desc);
    const bool recreate_needed = (memcmp(&desc, &current_desc, sizeof(D3D11_TEXTURE2D_DESC)) != 0);
    LOG_EVENT_COND("SDM", recreate_needed, "dxgit staging texture recreate_needed: {}",
                   recreate_needed);
    if (!recreate_needed) {
      return true;
    }

    // The descriptions are not consistent, we need to create a new
    // ID3D11Texture2D instance.
    stage_.Reset();
    surface_.Reset();
  } else {
    TRAA_DCHECK(!surface_);
  }

  _com_error error =
      device_.get_d3d_device()->CreateTexture2D(&desc, nullptr, stage_.GetAddressOf());
  if (error.Error() != S_OK || !stage_) {
    LOG_ERROR("failed to create a new ID3D11Texture2D as stage: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  error = stage_.As(&surface_);
  if (error.Error() != S_OK || !surface_) {
    LOG_ERROR("failed to convert ID3D11Texture2D to IDXGISurface: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  return true;
}

void dxgi_texture_staging::assert_stage_and_surface_are_same_object() {
  ComPtr<IUnknown> left;
  ComPtr<IUnknown> right;
  bool left_result = SUCCEEDED(stage_.As(&left));
  bool right_result = SUCCEEDED(surface_.As(&right));
  TRAA_DCHECK(left_result);
  TRAA_DCHECK(right_result);
  TRAA_DCHECK(left.Get() == right.Get());
}

bool dxgi_texture_staging::copy_from_texture(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                             ID3D11Texture2D *texture) {
  TRAA_DCHECK_GT(frame_info.AccumulatedFrames, 0);
  TRAA_DCHECK(texture);

  // AcquireNextFrame returns a CPU inaccessible IDXGIResource, so we need to
  // copy it to a CPU accessible staging ID3D11Texture2D.
  if (!initialize_stage(texture)) {
    return false;
  }

  device_.context()->CopyResource(static_cast<ID3D11Resource *>(stage_.Get()),
                                  static_cast<ID3D11Resource *>(texture));

  *rect() = {0};
  _com_error error = surface_->Map(rect(), DXGI_MAP_READ);
  if (error.Error() != S_OK) {
    *rect() = {0};
    LOG_ERROR("failed to map the IDXGISurface to a bitmap: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  return true;
}

bool dxgi_texture_staging::do_release() {
  _com_error error = surface_->Unmap();
  if (error.Error() != S_OK) {
    stage_.Reset();
    surface_.Reset();
  }
  // If using staging mode, we only need to recreate ID3D11Texture2D instance.
  // This will happen during next CopyFrom call. So this function always returns
  // true.
  return true;
}

} // namespace base
} // namespace traa
