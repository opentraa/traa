/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_frame_cgimage.h"

#include "base/checks.h"
#include "base/logger.h"

#include <AvailabilityMacros.h>

namespace traa {
namespace base {

// static
std::unique_ptr<desktop_frame_cgimage>
desktop_frame_cgimage::create_for_display(CGDirectDisplayID display_id) {
  // Create an image containing a snapshot of the display.
  scoped_cf_type_ref<CGImageRef> cg_image(CGDisplayCreateImage(display_id));
  if (!cg_image) {
    return nullptr;
  }

  return desktop_frame_cgimage::create_from_cgimage(cg_image);
}

// static
std::unique_ptr<desktop_frame_cgimage>
desktop_frame_cgimage::create_for_window(CGWindowID window_id) {
  scoped_cf_type_ref<CGImageRef> cg_image(
      CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, window_id,
                              kCGWindowImageBoundsIgnoreFraming));
  if (!cg_image) {
    return nullptr;
  }

  return desktop_frame_cgimage::create_from_cgimage(cg_image);
}

// static
std::unique_ptr<desktop_frame_cgimage>
desktop_frame_cgimage::create_from_cgimage(scoped_cf_type_ref<CGImageRef> cg_image) {
  // Verify that the image has 32-bit depth.
  int bits_per_pixel = static_cast<int>(CGImageGetBitsPerPixel(cg_image.get()));
  if (bits_per_pixel / 8 != desktop_frame::k_bytes_per_pixel) {
    LOG_ERROR("CGDisplayCreateImage() returned imaged with {} bits per pixel. Only 32-bit depth is "
              "supported.",
              bits_per_pixel);
    return nullptr;
  }

  // Request access to the raw pixel data via the image's DataProvider.
  CGDataProviderRef cg_provider = CGImageGetDataProvider(cg_image.get());
  TRAA_DCHECK(cg_provider);

  // CGDataProviderCopyData returns a new data object containing a copy of the provider’s
  // data.
  scoped_cf_type_ref<CFDataRef> cg_data(CGDataProviderCopyData(cg_provider));
  TRAA_DCHECK(cg_data);

  // CFDataGetBytePtr returns a read-only pointer to the bytes of a CFData object.
  uint8_t *data = const_cast<uint8_t *>(CFDataGetBytePtr(cg_data.get()));
  TRAA_DCHECK(data);

  desktop_size size(static_cast<int>(CGImageGetWidth(cg_image.get())),
                    static_cast<int>(CGImageGetHeight(cg_image.get())));
  int stride = static_cast<int>(CGImageGetBytesPerRow(cg_image.get()));

  std::unique_ptr<desktop_frame_cgimage> frame(
      new desktop_frame_cgimage(size, stride, data, cg_image, cg_data));

  CGColorSpaceRef cg_color_space = CGImageGetColorSpace(cg_image.get());
  if (cg_color_space) {
#if !defined(MAC_OS_X_VERSION_10_13) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_13
    scoped_cf_type_ref<CFDataRef> cf_icc_profile(CGColorSpaceCopyICCProfile(cg_color_space));
#else
    scoped_cf_type_ref<CFDataRef> cf_icc_profile(CGColorSpaceCopyICCData(cg_color_space));
#endif
    if (cf_icc_profile) {
      const uint8_t *data_as_byte =
          reinterpret_cast<const uint8_t *>(CFDataGetBytePtr(cf_icc_profile.get()));
      const size_t data_size = CFDataGetLength(cf_icc_profile.get());
      if (data_as_byte && data_size > 0) {
        frame->set_icc_profile(std::vector<uint8_t>(data_as_byte, data_as_byte + data_size));
      }
    }
  }

  return frame;
}

desktop_frame_cgimage::desktop_frame_cgimage(desktop_size size, int stride, uint8_t *data,
                                             scoped_cf_type_ref<CGImageRef> cg_image,
                                             scoped_cf_type_ref<CFDataRef> cg_data)
    : desktop_frame(size, stride, data, nullptr), cg_image_(cg_image), cg_data_(cg_data) {
  TRAA_DCHECK(cg_image_);
  TRAA_DCHECK(cg_data_);
}

desktop_frame_cgimage::~desktop_frame_cgimage() = default;

} // namespace base
} // namespace traa
