/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_THREAD_DESKTOP_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_THREAD_DESKTOP_H_

#include <windows.h>

#include <memory>

namespace traa {
namespace base {

class thread_desktop;
class scoped_thread_desktop {
public:
  scoped_thread_desktop();
  ~scoped_thread_desktop();

  scoped_thread_desktop(const scoped_thread_desktop &) = delete;
  scoped_thread_desktop &operator=(const scoped_thread_desktop &) = delete;

  // Returns true if `desktop` has the same desktop name as the currently
  // assigned desktop (if assigned) or as the initial desktop (if not assigned).
  // Returns false in any other case including failing Win32 APIs and
  // uninitialized desktop handles.
  bool is_same(const thread_desktop &other) const;

  // Reverts the calling thread to use the initial desktop.
  void revert();

  // Assigns `desktop` to be the calling thread. Returns true if the thread has
  // been switched to `desktop` successfully. Takes ownership of `desktop`.
  bool set_thread_desktop(thread_desktop *desktop);

private:
  // The desktop handle assigned to the calling thread by set_thread_desktop.
  std::unique_ptr<thread_desktop> assigned_;

  // The desktop handle assigned to the calling thread at creation.
  std::unique_ptr<thread_desktop> initial_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_THREAD_DESKTOP_H_
