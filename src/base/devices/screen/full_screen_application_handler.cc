/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/full_screen_application_handler.h"

#include "base/logger.h"

namespace traa {
namespace base {

full_screen_app_handler::full_screen_app_handler(desktop_capturer::source_id_t id)
    : source_id_(id) {}

desktop_capturer::source_id_t
full_screen_app_handler::find_full_screen_window(const desktop_capturer::source_list_t &,
                                                 int64_t) const {
  return 0;
}

desktop_capturer::source_id_t full_screen_app_handler::get_source_id() const { return source_id_; }

} // namespace base
} // namespace traa
