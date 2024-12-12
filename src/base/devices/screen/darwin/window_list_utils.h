/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef BASE_DEVICES_SCREEN_DARWIN_WINDOW_LIST_UTILS_H_
#define BASE_DEVICES_SCREEN_DARWIN_WINDOW_LIST_UTILS_H_

#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/function_view.h"

#include <ApplicationServices/ApplicationServices.h>

#include <string>

namespace traa {
namespace base {

// Converts a CFStringRef to a UTF-8 encoded std::string.
bool cfstring_to_utf8(const CFStringRef str16, std::string *str8);

// Get CFDictionaryRef from `id` and call `on_window` against it. This function
// returns false if native APIs fail, typically it indicates that the `id` does
// not represent a window. `on_window` will not be called if false is returned
// from this function.
bool get_window_ref(CGWindowID id, function_view<void(CFDictionaryRef)> on_window);

// Iterates all on-screen windows in decreasing z-order and sends them
// one-by-one to `on_window` function. If `on_window` returns false, this
// function returns immediately. get_window_list() returns false if native APIs
// failed. Menus, dock (if `only_zero_layer`), minimized windows (if
// `ignore_minimized` is true) and any windows which do not have a valid window
// id or title will be ignored.
bool get_window_list(function_view<bool(CFDictionaryRef)> on_window, bool ignore_minimized,
                     bool only_zero_layer);

// Another helper function to get the on-screen windows.
bool get_window_list(desktop_capturer::source_list_t *windows, bool ignore_minimized,
                     bool only_zero_layer);

// Returns true if the window is occupying a full screen.
bool is_window_full_screen(const desktop_configuration &desktop_config, CFDictionaryRef window);

// Returns true if the window is occupying a full screen.
bool is_window_full_screen(const desktop_configuration &desktop_config, CGWindowID id);

// Returns true if the `window` is on screen. This function returns false if
// native APIs fail.
bool is_window_on_screen(CFDictionaryRef window);

// Returns true if the window is on screen. This function returns false if
// native APIs fail or `id` cannot be found.
bool is_window_on_screen(CGWindowID id);

// Returns utf-8 encoded title of `window`. If `window` is not a window or no
// valid title can be retrieved, this function returns an empty string.
std::string get_window_title(CFDictionaryRef window);

// Returns utf-8 encoded title of window `id`. If `id` cannot be found or no
// valid title can be retrieved, this function returns an empty string.
std::string get_window_title(CGWindowID id);

// Returns utf-8 encoded owner name of `window`. If `window` is not a window or
// if no valid owner name can be retrieved, returns an empty string.
std::string get_window_owner_name(CFDictionaryRef window);

// Returns utf-8 encoded owner name of the given window `id`. If `id` cannot be
// found or if no valid owner name can be retrieved, returns an empty string.
std::string get_window_owner_name(CGWindowID id);

// Returns id of `window`. If `window` is not a window or the window id cannot
// be retrieved, this function returns k_window_id_null.
win_id_t get_window_id(CFDictionaryRef window);

// Returns the pid of the process owning `window`. Return 0 if `window` is not
// a window or no valid owner can be retrieved.
int get_window_owner_pid(CFDictionaryRef window);

// Returns the pid of the process owning the window `id`. Return 0 if `id`
// cannot be found or no valid owner can be retrieved.
int get_window_owner_pid(CGWindowID id);

// Returns the DIP to physical pixel scale at `position`. `position` is in
// *unscaled* system coordinate, i.e. it's device-independent and the primary
// monitor starts from (0, 0). If `position` is out of the system display, this
// function returns 1.
float get_scale_factor_at_position(const desktop_configuration &desktop_config,
                                   desktop_vector position);

// Returns the DIP to physical pixel scale factor of the window with `id`.
// The bounds of the window with `id` is in DIP coordinates and `size` is the
// CGImage size of the window with `id` in physical coordinates. Comparing them
// can give the current scale factor.
// If the window overlaps multiple monitors, OS will decide on which monitor the
// window is displayed and use its scale factor to the window. So this method
// still works.
float get_window_scale_factor(CGWindowID id, desktop_size size);

// Returns the bounds of `window`. If `window` is not a window or the bounds
// cannot be retrieved, this function returns an empty desktop_rect. The returned
// desktop_rect is in system coordinate, i.e. the primary monitor always starts
// from (0, 0).
// Deprecated: This function should be avoided in favor of the overload with
// MacDesktopConfiguration.
desktop_rect get_window_bounds(CFDictionaryRef window);

// Returns the bounds of window with `id`. If `id` does not represent a window
// or the bounds cannot be retrieved, this function returns an empty
// desktop_rect. The returned desktop_rect is in system coordinates.
// Deprecated: This function should be avoided in favor of the overload with
// MacDesktopConfiguration.
desktop_rect get_window_bounds(CGWindowID id);

} // namespace base
} // namespace traa

#endif // BASE_DEVICES_SCREEN_DARWIN_WINDOW_LIST_UTILS_H_
