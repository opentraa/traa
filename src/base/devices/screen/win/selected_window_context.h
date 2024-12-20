/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SELECTED_WINDOW_CONTEXT_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SELECTED_WINDOW_CONTEXT_H_

#include <windows.h>

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/win/capture_utils.h"

namespace traa {
namespace base {

class selected_window_context {
public:
  selected_window_context(HWND selected_window, desktop_rect selected_window_rect,
                          window_capture_helper_win *capture_helper);

  bool is_selected_window_valid() const;

  bool is_window_owned_by_selected_window(HWND window) const;
  bool is_window_overlapping_selected_window(HWND window) const;

  HWND selected_window() const;
  window_capture_helper_win *get_window_capture_helper() const;

private:
  const HWND selected_window_;
  const desktop_rect selected_window_rect_;
  window_capture_helper_win *const capture_helper_;
  DWORD selected_window_thread_id_;
  DWORD selected_window_process_id_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SELECTED_WINDOW_CONTEXT_H_
