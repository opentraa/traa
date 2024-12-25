/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_WIN_H_
#define TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_WIN_H_

#include "base/devices/screen/window_finder.h"

namespace traa {
namespace base {

// The implementation of window_finder for Windows.
class window_finder_win final : public window_finder {
public:
  window_finder_win();
  ~window_finder_win() override;

  // window_finder implementation.
  win_id_t get_window_under_point(desktop_vector point) override;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_WIN_H_