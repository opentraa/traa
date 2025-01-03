/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/window_finder_mac.h"

#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/darwin/desktop_configuration_monitor.h"
#include "base/devices/screen/darwin/window_list_utils.h"

#include <CoreFoundation/CoreFoundation.h>

#include <memory>
#include <utility>

namespace traa {
namespace base {

window_finder_mac::window_finder_mac(
    std::shared_ptr<desktop_configuration_monitor> configuration_monitor)
    : configuration_monitor_(std::move(configuration_monitor)) {}
window_finder_mac::~window_finder_mac() = default;

win_id_t window_finder_mac::get_window_under_point(desktop_vector point) {
  win_id_t id = k_window_id_null;
  get_window_list(
      [&id, point](CFDictionaryRef window) {
        desktop_rect bounds;
        bounds = get_window_bounds(window);
        if (bounds.contains(point)) {
          id = get_window_id(window);
          return false;
        }
        return true;
      },
      true, true);
  return id;
}

// static
std::unique_ptr<window_finder> window_finder::create(const window_finder::options &options) {
  return std::make_unique<window_finder_mac>(options.configuration_monitor);
}

} // namespace base
} // namespace traa
