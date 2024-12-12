/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/selected_window_context.h"

namespace traa {
namespace base {

selected_window_context::selected_window_context(HWND selected_window,
                                                 desktop_rect selected_window_rect,
                                                 window_capture_helper_win *capture_helper)
    : selected_window_(selected_window), selected_window_rect_(selected_window_rect),
      capture_helper_(capture_helper) {
  selected_window_thread_id_ =
      ::GetWindowThreadProcessId(selected_window, &selected_window_process_id_);
}

bool selected_window_context::is_selected_window_valid() const {
  return selected_window_thread_id_ != 0;
}

bool selected_window_context::is_window_owned_by_selected_window(HWND window) const {
  // This check works for drop-down menus & dialog pop-up windows.
  if (::GetAncestor(window, GA_ROOTOWNER) == selected_window_) {
    return true;
  }

  // Assume that all other windows are unrelated to the selected window.
  // This will cause some windows that are actually related to be missed,
  // e.g. context menus and tool-tips, but avoids the risk of capturing
  // unrelated windows. Using heuristics such as matching the thread and
  // process Ids suffers from false-positives, e.g. in multi-document
  // applications.

  return false;
}

bool selected_window_context::is_window_overlapping_selected_window(HWND window) const {
  return capture_utils::is_window_overlapping(window, selected_window_, selected_window_rect_);
}

HWND selected_window_context::selected_window() const { return selected_window_; }

window_capture_helper_win *selected_window_context::get_window_capture_helper() const {
  return capture_helper_;
}

} // namespace base
} // namespace traa
