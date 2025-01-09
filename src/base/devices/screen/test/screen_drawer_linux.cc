/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/screen_drawer_lock_posix.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/linux/x11/shared_x_display.h"

#include "base/devices/screen/rgba_color.h"
#include "base/devices/screen/test/screen_drawer.h"

#include "base/checks.h"
#include "base/system/sleep.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <string.h>

#include <memory>

namespace traa {
namespace base {

namespace {

// A screen_drawer implementation for X11.
class screen_drawer_linux final : public screen_drawer {
public:
  screen_drawer_linux();
  ~screen_drawer_linux() override;

  // screen_drawer interface.
  desktop_rect drawable_region() override;
  void draw_rectangle(desktop_rect rect, rgba_color color) override;
  void clear() override;
  void wait_for_pending_draws() override;
  bool may_draw_incomplete_shapes() override;
  win_id_t window_id() const override;

private:
  // Bring the window to the front, this can help to avoid the impact from other
  // windows or shadow effect.
  void bring_to_front();

  std::shared_ptr<shared_x_display> display_;
  int screen_num_;
  desktop_rect rect_;
  Window window_;
  GC context_;
  Colormap colormap_;
};

screen_drawer_linux::screen_drawer_linux() {
  display_ = shared_x_display::create_default();
  TRAA_CHECK(display_.get());
  screen_num_ = DefaultScreen(display_->display());
  XWindowAttributes root_attributes;
  if (!XGetWindowAttributes(display_->display(), RootWindow(display_->display(), screen_num_),
                            &root_attributes)) {
    TRAA_DCHECK_NOTREACHED() << "failed to get root window size.";
  }
  window_ = XCreateSimpleWindow(display_->display(), RootWindow(display_->display(), screen_num_),
                                0, 0, root_attributes.width, root_attributes.height, 0,
                                BlackPixel(display_->display(), screen_num_),
                                BlackPixel(display_->display(), screen_num_));
  XSelectInput(display_->display(), window_, StructureNotifyMask);
  XMapWindow(display_->display(), window_);
  while (true) {
    XEvent event;
    XNextEvent(display_->display(), &event);
    if (event.type == MapNotify) {
      break;
    }
  }
  XFlush(display_->display());
  Window child;
  int x, y;
  if (!XTranslateCoordinates(display_->display(), window_,
                             RootWindow(display_->display(), screen_num_), 0, 0, &x, &y, &child)) {
    TRAA_DCHECK_NOTREACHED() << "Failed to get window position.";
  }
  // Some window manager does not allow a window to cover two or more monitors.
  // So if the window is on the first monitor of a two-monitor system, the
  // second half won't be able to show up without changing configurations of WM,
  // and its drawable_region() is not accurate.
  rect_ = desktop_rect::make_ltrb(x, y, root_attributes.width, root_attributes.height);
  context_ = DefaultGC(display_->display(), screen_num_);
  colormap_ = DefaultColormap(display_->display(), screen_num_);
  bring_to_front();
  // Wait for window animations.
  sleep_ms(200);
}

screen_drawer_linux::~screen_drawer_linux() {
  XUnmapWindow(display_->display(), window_);
  XDestroyWindow(display_->display(), window_);
}

desktop_rect screen_drawer_linux::drawable_region() { return rect_; }

void screen_drawer_linux::draw_rectangle(desktop_rect rect, rgba_color color) {
  rect.translate(-rect_.left(), -rect_.top());
  XColor xcolor;
  // X11 does not support Alpha.
  // X11 uses 16 bits for each primary color, so we need to slightly normalize
  // a 8 bits channel to 16 bits channel, by setting the low 8 bits as its high
  // 8 bits to avoid a mismatch of color returned by capturer.
  xcolor.red = (color.red << 8) + color.red;
  xcolor.green = (color.green << 8) + color.green;
  xcolor.blue = (color.blue << 8) + color.blue;
  xcolor.flags = DoRed | DoGreen | DoBlue;
  XAllocColor(display_->display(), colormap_, &xcolor);
  XSetForeground(display_->display(), context_, xcolor.pixel);
  XFillRectangle(display_->display(), window_, context_, rect.left(), rect.top(), rect.width(),
                 rect.height());
  XFlush(display_->display());
}

void screen_drawer_linux::clear() { draw_rectangle(rect_, rgba_color(0, 0, 0)); }

// TODO(zijiehe): Find the right signal from X11 to indicate the finish of all
// pending paintings.
void screen_drawer_linux::wait_for_pending_draws() { sleep_ms(50); }

bool screen_drawer_linux::may_draw_incomplete_shapes() { return true; }

win_id_t screen_drawer_linux::window_id() const { return window_; }

void screen_drawer_linux::bring_to_front() {
  Atom state_above = XInternAtom(display_->display(), "_NET_WM_STATE_ABOVE", 1);
  Atom window_state = XInternAtom(display_->display(), "_NET_WM_STATE", 1);
  if (state_above == None || window_state == None) {
    // Fallback to use XRaiseWindow, it's not reliable if two windows are both
    // raise itself to the top.
    XRaiseWindow(display_->display(), window_);
    return;
  }

  XEvent event;
  memset(&event, 0, sizeof(event));
  event.type = ClientMessage;
  event.xclient.window = window_;
  event.xclient.message_type = window_state;
  event.xclient.format = 32;
  event.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
  event.xclient.data.l[1] = state_above;
  XSendEvent(display_->display(), RootWindow(display_->display(), screen_num_), False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

} // namespace

// static
std::unique_ptr<screen_drawer_lock> screen_drawer_lock::create() {
  return std::make_unique<screen_drawer_lock_posix>();
}

// static
std::unique_ptr<screen_drawer> screen_drawer::create() {
  if (shared_x_display::create_default().get()) {
    return std::make_unique<screen_drawer_linux>();
  }
  return nullptr;
}

} // namespace base
} // namespace traa
