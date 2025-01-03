/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_frame_provider.h"

#include "base/checks.h"
#include "base/devices/screen/darwin/desktop_frame_cgimage.h"
#include "base/devices/screen/darwin/desktop_frame_iosurface.h"

#include <utility>

namespace traa {
namespace base {

desktop_frame_provider::desktop_frame_provider(bool allow_iosurface)
    : allow_iosurface_(allow_iosurface) {}

desktop_frame_provider::~desktop_frame_provider() { release(); }

std::unique_ptr<desktop_frame>
desktop_frame_provider::take_latest_frame_for_display(CGDirectDisplayID display_id) {
  if (!allow_iosurface_ || !io_surfaces_[display_id]) {
    // Regenerate a snapshot. If iosurface is on it will be empty until the
    // stream handler is called.
    return desktop_frame_cgimage::create_for_display(display_id);
  }

  return io_surfaces_[display_id]->share();
}

void desktop_frame_provider::invalidate_iosurface(CGDirectDisplayID display_id,
                                                  scoped_cf_type_ref<IOSurfaceRef> io_surface) {
  if (!allow_iosurface_) {
    return;
  }

  std::unique_ptr<desktop_frame_iosurface> desktop_frame_iosurface =
      desktop_frame_iosurface::wrap(io_surface);

  io_surfaces_[display_id] = desktop_frame_iosurface
                                 ? shared_desktop_frame::wrap(std::move(desktop_frame_iosurface))
                                 : nullptr;
}

void desktop_frame_provider::release() {
  if (!allow_iosurface_) {
    return;
  }

  io_surfaces_.clear();
}

} // namespace base
} // namespace traa
