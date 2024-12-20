/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_CONFIGURATION_MONITOR_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_CONFIGURATION_MONITOR_H_

#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/thread_annotations.h"

#include <ApplicationServices/ApplicationServices.h>

#include <memory>
#include <mutex>
#include <set>

namespace traa {
namespace base {

// The class provides functions to synchronize capturing and display
// reconfiguring across threads, and the up-to-date MacDesktopConfiguration.
class desktop_configuration_monitor final {
public:
  desktop_configuration_monitor();
  ~desktop_configuration_monitor();

  desktop_configuration_monitor(const desktop_configuration_monitor &) = delete;
  desktop_configuration_monitor &operator=(const desktop_configuration_monitor &) = delete;

  // Returns the current desktop configuration.
  desktop_configuration get_desktop_configuration();

private:
  static void reconfigure_callback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags,
                                   void *user_parameter);
  void reconfigured(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags);

  std::mutex desktop_configuration_lock_;
  desktop_configuration desktop_configuration_ TRAA_GUARDED_BY(&desktop_configuration_lock_);
  std::set<CGDirectDisplayID> reconfiguring_displays_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_DESKTOP_CONFIGURATION_MONITOR_H_
