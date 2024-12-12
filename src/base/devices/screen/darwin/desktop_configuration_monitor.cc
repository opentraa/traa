/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_configuration_monitor.h"

#include "base/log/logger.h"

namespace traa {
namespace base {

desktop_configuration_monitor::desktop_configuration_monitor() {
  CGError err = CGDisplayRegisterReconfigurationCallback(
      desktop_configuration_monitor::reconfigure_callback, this);
  if (err != kCGErrorSuccess) {
    LOG_ERROR("CGDisplayRegisterReconfigurationCallback {}", static_cast<int>(err));
  }

  std::lock_guard<std::mutex> lock(desktop_configuration_lock_);
  desktop_configuration_ =
      desktop_configuration::current(desktop_configuration::COORDINATE_TOP_LEFT);
}

desktop_configuration_monitor::~desktop_configuration_monitor() {
  CGError err = CGDisplayRemoveReconfigurationCallback(
      desktop_configuration_monitor::reconfigure_callback, this);
  if (err != kCGErrorSuccess) {
    LOG_ERROR("CGDisplayRemoveReconfigurationCallback {}", static_cast<int>(err));
  }
}

desktop_configuration desktop_configuration_monitor::get_desktop_configuration() {
  std::lock_guard<std::mutex> lock(desktop_configuration_lock_);
  return desktop_configuration_;
}

// static
// This method may be called on any system thread.
void desktop_configuration_monitor::reconfigure_callback(CGDirectDisplayID display,
                                                         CGDisplayChangeSummaryFlags flags,
                                                         void *user_parameter) {
  desktop_configuration_monitor *monitor =
      reinterpret_cast<desktop_configuration_monitor *>(user_parameter);
  monitor->reconfigured(display, flags);
}

void desktop_configuration_monitor::reconfigured(CGDirectDisplayID display,
                                                 CGDisplayChangeSummaryFlags flags) {
  // TODO @sylar: optimize the LOG_EVENT
  LOG_EVENT("SDM", "desktop_configuration_monitor::reconfigured");
  LOG_INFO("reconfigured: DisplayID {}; ChangeSummaryFlags {}", display, flags);

  if (flags & kCGDisplayBeginConfigurationFlag) {
    reconfiguring_displays_.insert(display);
    return;
  }

  reconfiguring_displays_.erase(display);
  if (reconfiguring_displays_.empty()) {
    std::lock_guard<std::mutex> lock(desktop_configuration_lock_);
    desktop_configuration_ =
        desktop_configuration::current(desktop_configuration::COORDINATE_TOP_LEFT);
  }
}

} // namespace base
} // namespace traa
