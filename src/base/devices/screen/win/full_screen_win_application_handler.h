/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_FULL_SCREEN_WIN_APPLICATION_HANDLER_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_FULL_SCREEN_WIN_APPLICATION_HANDLER_H_

#include <memory>

#include "base/devices/screen/full_screen_application_handler.h"

namespace traa {
namespace base {

std::unique_ptr<full_screen_app_handler>
create_full_screen_app_handler(desktop_capturer::source_id_t source_id_t);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_FULL_SCREEN_WIN_APPLICATION_HANDLER_H_
