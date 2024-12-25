/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_X11_SHARED_X_DISPLAY_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_X11_SHARED_X_DISPLAY_H_

#include <map>
#include <mutex>
#include <string_view>
#include <vector>

#include "base/thread_annotations.h"

// Including Xlib.h will involve evil defines (Bool, Status, True, False), which
// easily conflict with other headers.
using x_display_t = struct _XDisplay;
using x_event_t = union _XEvent;

namespace traa {
namespace base {

// A ref-counted object to store XDisplay connection.
class shared_x_display : public std::enable_shared_from_this<shared_x_display> {
public:
  class x_evt_handler {
  public:
    virtual ~x_evt_handler() {}

    // Processes x_event_t. Returns true if the event has been handled.
    virtual bool on_x_event(const x_event_t &event) = 0;
  };

  // Creates a new X11 Display for the `display_name`. NULL is returned if X11
  // connection failed. Equivalent to create_default() when `display_name` is
  // empty.
  static std::shared_ptr<shared_x_display> create(std::string_view display_name);

  // Creates X11 Display connection for the default display (e.g. specified in
  // DISPLAY). NULL is returned if X11 connection failed.
  static std::shared_ptr<shared_x_display> create_default();

  x_display_t *display() { return display_; }

  // Adds a new event `handler` for x_event_t's of `type`.
  void add_x_event_handler(int type, x_evt_handler *handler);

  // Removes event `handler` added using `AddEventHandler`. Doesn't do anything
  // if `handler` is not registered.
  void remove_x_event_handler(int type, x_evt_handler *handler);

  // Processes pending XEvents, calling corresponding event handlers.
  void process_pending_x_events();

  void ignore_x_server_grabs();

  ~shared_x_display();

  shared_x_display(const shared_x_display &) = delete;
  shared_x_display &operator=(const shared_x_display &) = delete;

protected:
  // Takes ownership of `display`.
  explicit shared_x_display(x_display_t *display);

private:
  using x_event_handlers_map_t = std::map<int, std::vector<x_evt_handler *>>;

private:
  x_display_t *display_;

  std::mutex mutex_;

  x_event_handlers_map_t event_handlers_ TRAA_GUARDED_BY(mutex_);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_X11_SHARED_X_DISPLAY_H_
