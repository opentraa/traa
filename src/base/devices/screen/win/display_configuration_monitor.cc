/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/display_configuration_monitor.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/logger.h"

#include <windows.h>

namespace traa {
namespace base {

bool display_configuration_monitor::is_changed(desktop_capturer::source_id_t source_id) {
  desktop_rect rect = capture_utils::get_full_screen_rect();
  desktop_vector dpi = get_dpi_for_source_id(source_id);

  if (!initialized_) {
    initialized_ = true;
    rect_ = rect;
    source_dpis_.emplace(source_id, std::move(dpi));
    return false;
  }

  if (source_dpis_.find(source_id) == source_dpis_.end()) {
    // If this is the first time we've seen this source_id, use the current DPI
    // so the monitor does not indicate a change and possibly get reset.
    source_dpis_.emplace(source_id, dpi);
  }

  bool has_changed = false;
  if (!rect.equals(rect_) || !source_dpis_.at(source_id).equals(dpi)) {
    has_changed = true;
    rect_ = rect;
    source_dpis_.emplace(source_id, std::move(dpi));
  }

  return has_changed;
}

void display_configuration_monitor::reset() {
  initialized_ = false;
  source_dpis_.clear();
  rect_ = {};
}

desktop_vector
display_configuration_monitor::get_dpi_for_source_id(desktop_capturer::source_id_t source_id) {
  HMONITOR monitor = 0;
  if (source_id == k_screen_id_full) {
    // Get a handle to the primary monitor when capturing the full desktop.
    monitor = ::MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
  } else if (!capture_utils::get_hmonitor_from_device_index(source_id, &monitor)) {
    LOG_WARN("capture_utils::get_hmonitor_from_device_index failed.");
  }
  return capture_utils::get_dpi_for_monitor(monitor);
}

} // namespace base
} // namespace traa
