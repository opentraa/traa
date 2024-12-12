/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/window_finder_win.h"

#include <Windows.h>

#include <memory>

namespace traa {
namespace base {

window_finder_win::window_finder_win() = default;
window_finder_win::~window_finder_win() = default;

win_id_t window_finder_win::get_window_under_point(desktop_vector point) {
  HWND window = ::WindowFromPoint(POINT{point.x(), point.y()});
  if (!window) {
    return k_window_id_null;
  }

  // The difference between GA_ROOTOWNER and GA_ROOT can be found at
  // https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/Hirr_DkuZdw.
  // In short, we should use GA_ROOT, since we only care about the root window
  // but not the owner.
  window = ::GetAncestor(window, GA_ROOT);
  if (!window) {
    return k_window_id_null;
  }

  return reinterpret_cast<win_id_t>(window);
}

// static
std::unique_ptr<window_finder> window_finder::create(const window_finder::options &options) {
  return std::make_unique<window_finder_win>();
}

} // namespace base
} // namespace traa