/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_THREAD_DESKTOP_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_THREAD_DESKTOP_H_

#include <windows.h>

#include <string>

namespace traa {
namespace base {

class thread_desktop {
public:
  ~thread_desktop();

  thread_desktop(const thread_desktop &) = delete;
  thread_desktop &operator=(const thread_desktop &) = delete;

  // Returns the name of the thread_desktop represented by the object. Return false if
  // quering the name failed for any reason.
  bool get_name(std::wstring *desktop_name_out) const;

  // Returns true if `other` has the same name as this thread_desktop. Returns false
  // in any other case including failing Win32 APIs and uninitialized thread_desktop
  // handles.
  bool is_same(const thread_desktop &other) const;

  // Assigns the thread_desktop to the current thread. Returns false is the operation
  // failed for any reason.
  bool set_thread_desktop() const;

  // Returns the thread_desktop by its name or NULL if an error occurs.
  static thread_desktop *get_desktop(const wchar_t *desktop_name);

  // Returns the thread_desktop currently receiving user input or NULL if an error
  // occurs.
  static thread_desktop *get_input_desktop();

  // Returns the thread_desktop currently assigned to the calling thread or NULL if
  // an error occurs.
  static thread_desktop *get_thread_desktop();

private:
  thread_desktop(HDESK desk, bool own);

  // The thread_desktop handle.
  HDESK desk_;

  // True if `desk_` must be closed on teardown.
  bool own_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_THREAD_DESKTOP_H_
