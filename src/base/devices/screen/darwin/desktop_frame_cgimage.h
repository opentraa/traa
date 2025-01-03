/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_CGIMAGE_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_CGIMAGE_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/sdk/objc/helpers/scoped_cftyperef.h"

#include <CoreGraphics/CoreGraphics.h>

#include <memory>

namespace traa {
namespace base {

class desktop_frame_cgimage final : public desktop_frame {
public:
  // Create an image containing a snapshot of the display at the time this is
  // being called.
  static std::unique_ptr<desktop_frame_cgimage> create_for_display(CGDirectDisplayID display_id);

  // Create an image containing a snaphot of the given window at the time this
  // is being called. This also works when the window is overlapped or in
  // another workspace.
  static std::unique_ptr<desktop_frame_cgimage> create_for_window(CGWindowID window_id);

  static std::unique_ptr<desktop_frame_cgimage>
  create_from_cgimage(scoped_cf_type_ref<CGImageRef> cg_image);

  ~desktop_frame_cgimage() override;

  desktop_frame_cgimage(const desktop_frame_cgimage &) = delete;
  desktop_frame_cgimage &operator=(const desktop_frame_cgimage &) = delete;

private:
  // This constructor expects `cg_image` to hold a non-null CGImageRef.
  desktop_frame_cgimage(desktop_size size, int stride, uint8_t *data,
                        scoped_cf_type_ref<CGImageRef> cg_image,
                        scoped_cf_type_ref<CFDataRef> cg_data);

  const scoped_cf_type_ref<CGImageRef> cg_image_;
  const scoped_cf_type_ref<CFDataRef> cg_data_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_CGIMAGE_H_
