/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_H_
#define MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_H_

#include <ApplicationServices/ApplicationServices.h>

#include <vector>

#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

// Describes the configuration of a specific display.
struct display_configuration {
  display_configuration();
  display_configuration(const display_configuration &other);
  display_configuration(display_configuration &&other);
  ~display_configuration();

  display_configuration &operator=(const display_configuration &other);
  display_configuration &operator=(display_configuration &&other);

  // Cocoa identifier for this display.
  CGDirectDisplayID id = 0;

  // Bounds of this display in Density-Independent Pixels (DIPs).
  desktop_rect bounds;

  // Bounds of this display in physical pixels.
  desktop_rect pixel_bounds;

  // Scale factor from DIPs to physical pixels.
  float dip_to_pixel_scale = 1.0f;

  // Display type, built-in or external.
  bool is_builtin;
};

using display_configuration_array_t = std::vector<display_configuration> ;

// Describes the configuration of the whole desktop.
struct desktop_configuration {
  // Used to request bottom-up or top-down coordinates.
  enum coordinate_origin { COORDINATE_BOTTOM_LEFT, COORDINATE_TOP_LEFT };

  desktop_configuration();
  desktop_configuration(const desktop_configuration &other);
  desktop_configuration(desktop_configuration &&other);
  ~desktop_configuration();

  desktop_configuration &operator=(const desktop_configuration &other);
  desktop_configuration &operator=(desktop_configuration &&other);

  // Returns the desktop & display configurations.
  // If COORDINATE_BOTTOM_LEFT is used, the output is in Cocoa-style "bottom-up"
  // (the origin is the bottom-left of the primary monitor, and coordinates
  // increase as you move up the screen). Otherwise, the configuration will be
  // converted to follow top-left coordinate system as Windows and X11.
  static desktop_configuration current(coordinate_origin origin);

  // Returns true if the given desktop configuration equals this one.
  bool equals(const desktop_configuration &other);

  // If `id` corresponds to the built-in display, return its configuration,
  // otherwise return the configuration for the display with the specified id,
  // or nullptr if no such display exists.
  const display_configuration *find_by_id(CGDirectDisplayID id);

  // Bounds of the desktop excluding monitors with DPI settings different from
  // the main monitor. In Density-Independent Pixels (DIPs).
  desktop_rect bounds;

  // Same as bounds, but expressed in physical pixels.
  desktop_rect pixel_bounds;

  // Scale factor from DIPs to physical pixels.
  float dip_to_pixel_scale = 1.0f;

  // Configurations of the displays making up the desktop area.
  display_configuration_array_t displays;
};

} // namespace base
} // namespace traa

#endif // MODULES_DESKTOP_CAPTURE_MAC_DESKTOP_CONFIGURATION_H_
