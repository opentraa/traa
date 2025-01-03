/*
 *  Copyright (c) 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_UTILS_H_

#include "base/devices/screen/darwin/desktop_frame_cgimage.h"
#include "base/devices/screen/desktop_frame.h"

#include <memory>

namespace traa {
namespace base {

std::unique_ptr<desktop_frame>
create_desktop_frame_from_cgimage(scoped_cf_type_ref<CGImageRef> cg_image);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_FRAME_UTILS_H_