/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/linux/x11/shared_x_display.h"

#include "base/logger.h"

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <algorithm>

namespace webrtc {

shared_x_display::shared_x_display(x_display_t *display) : display_(display) {}

shared_x_display::~shared_x_display() { XCloseDisplay(display_); }

// static
std::shared_ptr<shared_x_display> shared_x_display::create(std::string_view display_name) {
  x_display_t *display =
      XOpenDisplay(display_name.empty() ? NULL : std::string(display_name).c_str());
  if (!display) {
    LOG_ERROR("Unable to open display");
    return nullptr;
  }
  return std::make_shared<shared_x_display>(display);
}

// static
std::shared_ptr<shared_x_display> shared_x_display::create_default() {
  return create(std::string());
}

void shared_x_display::add_x_event_handler(int type, x_evt_handler *handler) {
  std::lock_guard<std::mutex> lock(&mutex_);
  event_handlers_[type].push_back(handler);
}

void shared_x_display::remove_x_event_handler(int type, x_evt_handler *handler) {
  std::lock_guard<std::mutex> lock(&mutex_);
  x_event_handlers_map_t::iterator handlers = event_handlers_.find(type);
  if (handlers == event_handlers_.end())
    return;

  std::vector<x_evt_handler *>::iterator new_end =
      std::remove(handlers->second.begin(), handlers->second.end(), handler);
  handlers->second.erase(new_end, handlers->second.end());

  // Check if no handlers left for this event.
  if (handlers->second.empty())
    event_handlers_.erase(handlers);
}

void shared_x_display::process_pending_x_events() {
  // Hold reference to `this` to prevent it from being destroyed while
  // processing events.
  std::shared_ptr<shared_x_display> self = std::shared_from_this();

  // Protect access to `event_handlers_` after incrementing the refcount for
  // `this` to ensure the instance is still valid when the lock is acquired.
  std::lock_guard<std::mutex> lock(&mutex_);

  // Find the number of events that are outstanding "now."  We don't just loop
  // on XPending because we want to guarantee this terminates.
  int events_to_process = XPending(display());
  x_event_t e;

  for (int i = 0; i < events_to_process; i++) {
    XNextEvent(display(), &e);
    x_event_handlers_map_t::iterator handlers = event_handlers_.find(e.type);
    if (handlers == event_handlers_.end())
      continue;
    for (std::vector<x_evt_handler *>::iterator it = handlers->second.begin();
         it != handlers->second.end(); ++it) {
      if ((*it)->on_x_event(e))
        break;
    }
  }
}

void shared_x_display::ignore_x_server_grabs() {
  int test_event_base = 0;
  int test_error_base = 0;
  int major = 0;
  int minor = 0;
  if (XTestQueryExtension(display(), &test_event_base, &test_error_base, &major, &minor)) {
    XTestGrabControl(display(), true);
  }
}

} // namespace webrtc
