/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_texture_mapping.h"

#include "base/devices/screen/win/desktop_capture_utils.h"
#include "base/logger.h"

#include <comdef.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace traa {
namespace base {

dxgi_texture_mapping::dxgi_texture_mapping(IDXGIOutputDuplication *duplication)
    : duplication_(duplication) {}

dxgi_texture_mapping::~dxgi_texture_mapping() = default;

bool dxgi_texture_mapping::copy_from_texture(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                             ID3D11Texture2D *texture) {
  *rect() = {0};
  _com_error error = duplication_->MapDesktopSurface(rect());
  if (error.Error() != S_OK) {
    *rect() = {0};
    LOG_ERROR("failed to map the IDXGIOutputDuplication to a bitmap: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  return true;
}

bool dxgi_texture_mapping::do_release() {
  _com_error error = duplication_->UnMapDesktopSurface();
  if (error.Error() != S_OK) {
    LOG_ERROR("failed to unmap the IDXGIOutputDuplication: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }
  return true;
}

} // namespace base
} // namespace traa
