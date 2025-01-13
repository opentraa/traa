/*
 *  Copyright (c) 2021 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capture_metrics_helper.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/system/metrics.h"

namespace traa {
namespace base {
namespace {
// This enum is logged via UMA so entries should not be reordered or have their
// values changed. This should also be kept in sync with the values in the
// DesktopCapturerId namespace.
enum class sequential_desktop_capturer_id {
  k_unknown = 0,
  k_wgc_capturer_win = 1,
  k_screen_capturer_win_magnifier = 2,
  k_window_capturer_win_gdi = 3,
  k_screen_capturer_win_gdi = 4,
  k_screen_capturer_win_directx = 5,
  k_max_value = k_screen_capturer_win_directx
};
} // namespace

void record_capturer_impl(uint32_t capturer_id) {
  sequential_desktop_capturer_id sequential_id;
  switch (capturer_id) {
  case desktop_capture_id::k_capture_wgc:
    sequential_id = sequential_desktop_capturer_id::k_wgc_capturer_win;
    break;
  case desktop_capture_id::k_capture_mag:
    sequential_id = sequential_desktop_capturer_id::k_screen_capturer_win_magnifier;
    break;
  case desktop_capture_id::k_capture_gdi_screen:
    sequential_id = sequential_desktop_capturer_id::k_screen_capturer_win_gdi;
    break;
  case desktop_capture_id::k_capture_dxgi:
    sequential_id = sequential_desktop_capturer_id::k_screen_capturer_win_directx;
    break;
  case desktop_capture_id::k_capture_unknown:
  default:
    sequential_id = sequential_desktop_capturer_id::k_unknown;
  }

  TRAA_HISTOGRAM_ENUMERATION("WebRTC.DesktopCapture.Win.DesktopCapturerImpl",
                             static_cast<int>(sequential_id),
                             static_cast<int>(sequential_desktop_capturer_id::k_max_value));
}

} // namespace base
} // namespace traa