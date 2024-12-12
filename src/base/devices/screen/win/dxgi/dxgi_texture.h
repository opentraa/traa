/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

#include <d3d11.h>
#include <dxgi1_2.h>

#include <memory>

namespace traa {
namespace base {

// A texture copied or mapped from a DXGI_OUTDUPL_FRAME_INFO and IDXGIResource.
class dxgi_texture {
public:
  // Creates a dxgi_texture instance, which represents the `desktop_size` area of
  // entire screen -- usually a monitor on the system.
  dxgi_texture();

  virtual ~dxgi_texture();

  // Copies selected regions of a frame represented by frame_info and resource.
  // Returns false if anything wrong.
  bool copy_from(const DXGI_OUTDUPL_FRAME_INFO &frame_info, IDXGIResource *resource);

  const desktop_size &get_desktop_size() const { return desktop_size_; }

  uint8_t *bits() const { return static_cast<uint8_t *>(rect_.pBits); }

  int pitch() const { return static_cast<int>(rect_.Pitch); }

  // Releases the resource currently holds by this instance. Returns false if
  // anything wrong, and this instance should be deprecated in this state. bits,
  // pitch and as_desktop_frame are only valid after a success copy_from() call,
  // but before release() call.
  bool release();

  // Returns a desktop_frame snapshot of a dxgi_texture instance. This
  // desktop_frame is used to copy a dxgi_texture content to another desktop_frame
  // only. And it should not outlive its dxgi_texture instance.
  const desktop_frame &as_desktop_frame();

protected:
  DXGI_MAPPED_RECT *rect();

  virtual bool copy_from_texture(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                 ID3D11Texture2D *texture) = 0;

  virtual bool do_release() = 0;

private:
  DXGI_MAPPED_RECT rect_ = {0};
  desktop_size desktop_size_;
  std::unique_ptr<desktop_frame> frame_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_TEXTURE_H_
