/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_PROVIDER_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_PROVIDER_H_

#include "base/devices/screen/shared_desktop_frame.h"
#include "base/sdk/objc/helpers/scoped_cftyperef.h"

#include <CoreGraphics/CoreGraphics.h>
#include <IOSurface/IOSurface.h>

#include <map>
#include <memory>

namespace traa {
namespace base {

class desktop_frame_provider {
public:
  explicit desktop_frame_provider(bool allow_iosurface);
  ~desktop_frame_provider();

  desktop_frame_provider(const desktop_frame_provider &) = delete;
  desktop_frame_provider &operator=(const desktop_frame_provider &) = delete;

  // The caller takes ownership of the returned desktop frame. Otherwise
  // returns null if `display_id` is invalid or not ready. Note that this
  // function does not remove the frame from the internal container. Caller
  // has to call the Release function.
  std::unique_ptr<desktop_frame> take_latest_frame_for_display(CGDirectDisplayID display_id);

  // OS sends the latest IOSurfaceRef through
  // CGDisplayStreamFrameAvailableHandler callback; we store it here.
  void invalidate_iosurface(CGDirectDisplayID display_id,
                            scoped_cf_type_ref<IOSurfaceRef> io_surface);

  // Expected to be called before stopping the CGDisplayStreamRef streams.
  void release();

  bool allow_iosurface() const { return allow_iosurface_; }

private:
  const bool allow_iosurface_;

  // Most recent IOSurface that contains a capture of matching display.
  std::map<CGDirectDisplayID, std::unique_ptr<shared_desktop_frame>> io_surfaces_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_PROVIDER_H_
