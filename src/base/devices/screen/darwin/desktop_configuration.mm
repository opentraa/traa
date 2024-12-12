/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/desktop_configuration.h"

#include <Cocoa/Cocoa.h>

#include <algorithm>
#include <math.h>

namespace traa {
namespace base {

namespace {

desktop_rect to_desktop_rect(const NSRect &ns_rect) {
  return desktop_rect::make_ltrb(static_cast<int>(floor(ns_rect.origin.x)),
                                 static_cast<int>(floor(ns_rect.origin.y)),
                                 static_cast<int>(ceil(ns_rect.origin.x + ns_rect.size.width)),
                                 static_cast<int>(ceil(ns_rect.origin.y + ns_rect.size.height)));
}

// Inverts the position of `rect` from bottom-up coordinates to top-down,
// relative to `bounds`.
void invert_rect_y_origin(const desktop_rect &bounds, desktop_rect *rect) {
  *rect = desktop_rect::make_xywh(rect->left(), bounds.bottom() - rect->bottom(), rect->width(),
                                  rect->height());
}

display_configuration get_configuration_for_screen(NSScreen *screen) {
  display_configuration display_config;

  // Fetch the NSScreenNumber, which is also the CGDirectDisplayID.
  NSDictionary *device_description = [screen deviceDescription];
  display_config.id = static_cast<CGDirectDisplayID>(
      [[device_description objectForKey:@"NSScreenNumber"] intValue]);

  // Determine the display's logical & physical dimensions.
  NSRect ns_bounds = [screen frame];
  display_config.bounds = to_desktop_rect(ns_bounds);

  display_config.dip_to_pixel_scale = [screen backingScaleFactor];
  NSRect ns_pixel_bounds = [screen convertRectToBacking:ns_bounds];
  display_config.pixel_bounds = to_desktop_rect(ns_pixel_bounds);

  // Determine if the display is built-in or external.
  display_config.is_builtin = CGDisplayIsBuiltin(display_config.id);

  return display_config;
}

} // namespace

display_configuration::display_configuration() = default;
display_configuration::display_configuration(const display_configuration &other) = default;
display_configuration::display_configuration(display_configuration &&other) = default;
display_configuration::~display_configuration() = default;

display_configuration &
display_configuration::operator=(const display_configuration &other) = default;
display_configuration &display_configuration::operator=(display_configuration &&other) = default;

desktop_configuration::desktop_configuration() = default;
desktop_configuration::desktop_configuration(const desktop_configuration &other) = default;
desktop_configuration::desktop_configuration(desktop_configuration &&other) = default;
desktop_configuration::~desktop_configuration() = default;

desktop_configuration &
desktop_configuration::operator=(const desktop_configuration &other) = default;
desktop_configuration &desktop_configuration::operator=(desktop_configuration &&other) = default;

// static
desktop_configuration desktop_configuration::current(coordinate_origin origin) {
  desktop_configuration desktop_config;

  NSArray *screens = [NSScreen screens];

  // Iterator over the monitors, adding the primary monitor and monitors whose
  // DPI match that of the primary monitor.
  for (NSUInteger i = 0; i < [screens count]; ++i) {
    display_configuration display_config = get_configuration_for_screen([screens objectAtIndex:i]);

    if (i == 0)
      desktop_config.dip_to_pixel_scale = display_config.dip_to_pixel_scale;

    // Cocoa uses bottom-up coordinates, so if the caller wants top-down then
    // we need to invert the positions of secondary monitors relative to the
    // primary one (the primary monitor's position is (0,0) in both systems).
    if (i > 0 && origin == COORDINATE_TOP_LEFT) {
      invert_rect_y_origin(desktop_config.displays[0].bounds, &display_config.bounds);
      // `display_bounds` is density dependent, so we need to convert the
      // primay monitor's position into the secondary monitor's density context.
      float scaling_factor =
          display_config.dip_to_pixel_scale / desktop_config.displays[0].dip_to_pixel_scale;
      desktop_rect primary_bounds = desktop_rect::make_ltrb(
          desktop_config.displays[0].pixel_bounds.left() * scaling_factor,
          desktop_config.displays[0].pixel_bounds.top() * scaling_factor,
          desktop_config.displays[0].pixel_bounds.right() * scaling_factor,
          desktop_config.displays[0].pixel_bounds.bottom() * scaling_factor);
      invert_rect_y_origin(primary_bounds, &display_config.pixel_bounds);
    }

    // Add the display to the configuration.
    desktop_config.displays.push_back(display_config);

    // Update the desktop bounds to account for this display, unless the current
    // display uses different DPI settings.
    if (display_config.dip_to_pixel_scale == desktop_config.dip_to_pixel_scale) {
      desktop_config.bounds.union_with(display_config.bounds);
      desktop_config.pixel_bounds.union_with(display_config.pixel_bounds);
    }
  }

  return desktop_config;
}

// For convenience of comparing display_configuration_array in
// desktop_configuration::equals.
bool operator==(const display_configuration &left, const display_configuration &right) {
  return left.id == right.id && left.bounds.equals(right.bounds) &&
         left.pixel_bounds.equals(right.pixel_bounds) &&
         left.dip_to_pixel_scale == right.dip_to_pixel_scale;
}

bool desktop_configuration::equals(const desktop_configuration &other) {
  return bounds.equals(other.bounds) && pixel_bounds.equals(other.pixel_bounds) &&
         dip_to_pixel_scale == other.dip_to_pixel_scale && displays == other.displays;
}

const display_configuration *desktop_configuration::find_by_id(CGDirectDisplayID id) {
  bool is_builtin = CGDisplayIsBuiltin(id);
  for (display_configuration_array_t::const_iterator it = displays.begin(); it != displays.end();
       ++it) {
    // The MBP having both discrete and integrated graphic cards will do
    // automate graphics switching by default. When it switches from discrete to
    // integrated one, the current display ID of the built-in display will
    // change and this will cause screen capture stops.
    // So make screen capture of built-in display continuing even if its display
    // ID is changed.
    if ((is_builtin && it->is_builtin) || (!is_builtin && it->id == id))
      return &(*it);
  }
  return NULL;
}

} // namespace base
} // namespace traa
