/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_TYPES_H_
#define MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_TYPES_H_

#include <stdint.h>

namespace traa {
namespace base {

enum class capture_type { window, screen, any_content };

// Type used to identify windows on the desktop. Values are platform-specific:
//   - On Windows: HWND cast to intptr_t.
//   - On Linux (with X11): X11 Window (unsigned long) type cast to intptr_t.
//   - On OSX: integer window number.
using win_id_t = intptr_t;

constexpr win_id_t k_window_id_null = 0;

constexpr int64_t k_display_id_invalid = -1;

// Type used to identify screens on the desktop. Values are platform-specific:
//   - On Windows: integer display device index.
//   - On OSX: CGDirectDisplayID cast to intptr_t.
//   - On Linux (with X11): TBD.
// On Windows, screen_id_t is implementation dependent: sending a screen_id_t from one
// implementation to another usually won't work correctly.
using screen_id_t = intptr_t;

// The screen id corresponds to all screen combined together.
constexpr screen_id_t k_screen_id_full = -1;

constexpr screen_id_t k_screen_id_invalid = -2;

// Integers to attach to each desktop_frame to differentiate the generator of
// the frame.
namespace desktop_capture_id {
constexpr uint32_t create_four_cc(char a, char b, char c, char d) {
  return ((static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) |
          (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24));
}

constexpr uint32_t k_capture_unknown = 0;
constexpr uint32_t k_capture_wgc = 1;
constexpr uint32_t k_capture_mag = 2;
constexpr uint32_t k_capture_gdi_win = 3;
constexpr uint32_t k_capture_gdi_screen = create_four_cc('G', 'D', 'I', ' ');
constexpr uint32_t k_capture_dxgi = create_four_cc('D', 'X', 'G', 'I');
constexpr uint32_t k_capture_x11 = create_four_cc('X', '1', '1', ' ');
constexpr uint32_t k_capture_wayland = create_four_cc('W', 'L', ' ', ' ');
} // namespace desktop_capture_id

} // namespace base
} // namespace traa

#endif // MODULES_DESKTOP_CAPTURE_DESKTOP_CAPTURE_TYPES_H_
