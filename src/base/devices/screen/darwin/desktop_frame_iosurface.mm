/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_frame_iosurface.h"

#include "base/checks.h"
#include "base/logger.h"

namespace traa {
namespace base {

// static
std::unique_ptr<desktop_frame_iosurface>
desktop_frame_iosurface::wrap(scoped_cf_type_ref<IOSurfaceRef> io_surface) {
  if (!io_surface) {
    return nullptr;
  }

  IOSurfaceIncrementUseCount(io_surface.get());
  IOReturn status = IOSurfaceLock(io_surface.get(), kIOSurfaceLockReadOnly, nullptr);
  if (status != kIOReturnSuccess) {
    LOG_ERROR("failed to lock the IOSurface with status {}", status);
    IOSurfaceDecrementUseCount(io_surface.get());
    return nullptr;
  }

  // Verify that the image has 32-bit depth.
  int bytes_per_pixel = static_cast<int>(IOSurfaceGetBytesPerElement(io_surface.get()));
  if (bytes_per_pixel != desktop_frame::k_bytes_per_pixel) {
    LOG_ERROR("CGDisplayStream handler returned IOSurface with {} bits per pixel. Only 32-bit "
              "depth is supported.",
              8 * bytes_per_pixel);
    IOSurfaceUnlock(io_surface.get(), kIOSurfaceLockReadOnly, nullptr);
    IOSurfaceDecrementUseCount(io_surface.get());
    return nullptr;
  }

  return std::unique_ptr<desktop_frame_iosurface>(new desktop_frame_iosurface(io_surface));
}

desktop_frame_iosurface::desktop_frame_iosurface(scoped_cf_type_ref<IOSurfaceRef> io_surface)
    : desktop_frame(
          desktop_size(static_cast<int>(IOSurfaceGetWidth(io_surface.get())), static_cast<int>(IOSurfaceGetHeight(io_surface.get()))),
          static_cast<int>(IOSurfaceGetBytesPerRow(io_surface.get())),
          static_cast<uint8_t *>(IOSurfaceGetBaseAddress(io_surface.get())), nullptr),
      io_surface_(io_surface) {
  TRAA_DCHECK(io_surface_);
}

desktop_frame_iosurface::~desktop_frame_iosurface() {
  IOSurfaceUnlock(io_surface_.get(), kIOSurfaceLockReadOnly, nullptr);
  IOSurfaceDecrementUseCount(io_surface_.get());
}

} // namespace base
} // namespace traa
