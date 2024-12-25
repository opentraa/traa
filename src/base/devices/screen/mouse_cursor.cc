/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/mouse_cursor.h"

namespace traa {
namespace base {

mouse_cursor::mouse_cursor() {}

mouse_cursor::mouse_cursor(desktop_frame *image, const desktop_vector &hotspot)
    : image_(image), hotspot_(hotspot) {}

mouse_cursor::~mouse_cursor() {}

// static
mouse_cursor *mouse_cursor::copy_of(const mouse_cursor &cursor) {
  return cursor.image()
             ? new mouse_cursor(basic_desktop_frame::copy_of(*cursor.image()), cursor.hotspot())
             : new mouse_cursor();
}

} // namespace base
} // namespace traa