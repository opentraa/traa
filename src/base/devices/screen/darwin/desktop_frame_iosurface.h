/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_IOSURFACE_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_IOSURFACE_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/sdk/objc/helpers/scoped_cftyperef.h"

#include <CoreGraphics/CoreGraphics.h>
#include <IOSurface/IOSurface.h>

#include <memory>

namespace traa {
namespace base {

class desktop_frame_iosurface final : public desktop_frame {
public:
  // Lock an IOSurfaceRef containing a snapshot of a display. Return NULL if
  // failed to lock.
  static std::unique_ptr<desktop_frame_iosurface> wrap(scoped_cf_type_ref<IOSurfaceRef> io_surface);

  ~desktop_frame_iosurface() override;

  desktop_frame_iosurface(const desktop_frame_iosurface &) = delete;
  desktop_frame_iosurface &operator=(const desktop_frame_iosurface &) = delete;

private:
  // This constructor expects `io_surface` to hold a non-null IOSurfaceRef.
  explicit desktop_frame_iosurface(scoped_cf_type_ref<IOSurfaceRef> io_surface);

  const scoped_cf_type_ref<IOSurfaceRef> io_surface_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_IOSURFACE_H_