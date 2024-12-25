/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_DESKTOP_FRAME_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_DESKTOP_FRAME_H_

#include <d3d11.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

// desktop_frame implementation used by capturers that use the
// Windows.Graphics.Capture API.
class wgc_desktop_frame final : public desktop_frame {
public:
  // wgc_desktop_frame receives an rvalue reference to the `image_data` vector
  // so that it can take ownership of it (and avoid a copy).
  wgc_desktop_frame(desktop_size size, int stride, std::vector<uint8_t> &&image_data);

  wgc_desktop_frame(const wgc_desktop_frame &) = delete;
  wgc_desktop_frame &operator=(const wgc_desktop_frame &) = delete;

  ~wgc_desktop_frame() override;

private:
  std::vector<uint8_t> image_data_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_DESKTOP_FRAME_H_
