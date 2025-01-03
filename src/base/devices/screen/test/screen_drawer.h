/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_H_

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/rgba_color.h"

namespace traa {
namespace base {

// A cross-process lock to ensure only one screen_drawer can be used at a certain
// time.
class screen_drawer_lock {
public:
  virtual ~screen_drawer_lock();

  static std::unique_ptr<screen_drawer_lock> create();

protected:
  screen_drawer_lock();
};

// A set of basic platform dependent functions to draw various shapes on the
// screen.
class screen_drawer {
public:
  // Creates a screen_drawer for the current platform, returns nullptr if no
  // screen_drawer implementation available.
  // If the implementation cannot guarantee two screen_drawer instances won't
  // impact each other, this function may block current thread until another
  // screen_drawer has been destroyed.
  static std::unique_ptr<screen_drawer> create();

  screen_drawer();
  virtual ~screen_drawer();

  // Returns the region inside which draw_rectangle() function are expected to
  // work, in capturer coordinates (assuming ScreenCapturer::SelectScreen has
  // not been called). This region may exclude regions of the screen reserved by
  // the OS for things like menu bars or app launchers. The desktop_rect is in
  // system coordinate, i.e. the primary monitor always starts from (0, 0).
  virtual desktop_rect drawable_region() = 0;

  // Draws a rectangle to cover `rect` with `color`. Note, rect.bottom() and
  // rect.right() two lines are not included. The part of `rect` which is out of
  // drawable_region() will be ignored.
  virtual void draw_rectangle(desktop_rect rect, rgba_color color) = 0;

  // Clears all content on the screen by filling the area with black.
  virtual void clear() = 0;

  // Blocks current thread until OS finishes previous draw_rectangle() actions.
  // ScreenCapturer should be able to capture the changes after this function
  // finish.
  virtual void wait_for_pending_draws() = 0;

  // Returns true if incomplete shapes previous actions required may be drawn on
  // the screen after a WaitForPendingDraws() call. i.e. Though the complete
  // shapes will eventually be drawn on the screen, due to some OS limitations,
  // these shapes may be partially appeared sometimes.
  virtual bool may_draw_incomplete_shapes() = 0;

  // Returns the id of the drawer window. This function returns kNullWindowId if
  // the implementation does not draw on a window of the system.
  virtual win_id_t window_id() const = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_H_
