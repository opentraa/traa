/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DISPLAY_CONFIGURATION_MONITOR_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DISPLAY_CONFIGURATION_MONITOR_H_

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

// A passive monitor to detect the change of display configuration on a Windows
// system.
// TODO(zijiehe): Also check for pixel format changes.
class display_configuration_monitor {
public:
  // Checks whether the display configuration has changed since the last time
  // is_changed() was called. |source_id| is used to observe changes for a
  // specific display or all displays if kFullDesktopScreenId is passed in.
  // Returns false if object was reset() or if is_changed() has not been called.
  bool is_changed(desktop_capturer::source_id_t source_id);

  // Resets to the initial state.
  void reset();

private:
  desktop_vector get_dpi_for_source_id(desktop_capturer::source_id_t source_id);

  // Represents the size of the desktop which includes all displays.
  desktop_rect rect_;

  // Tracks the DPI for each display being captured. We need to track for each
  // display as each one can be configured to use a different DPI which will not
  // be reflected in calls to get the system DPI.
  std::map<desktop_capturer::source_id_t, desktop_vector> source_dpis_;

  // Indicates whether |rect_| and |source_dpis_| have been initialized. This is
  // used to prevent the monitor instance from signaling 'is_changed()' before
  // the initial values have been set.
  bool initialized_ = false;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DISPLAY_CONFIGURATION_MONITOR_H_
