/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_WINDOW_CAPTURER_WIN_GDI_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_WINDOW_CAPTURER_WIN_GDI_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/window_finder_win.h"

#include <map>
#include <memory>
#include <vector>

namespace traa {
namespace base {

class window_capturer_win_gdi : public desktop_capturer {
public:
  explicit window_capturer_win_gdi(bool enumerate_current_process_windows);

  // Disallow copy and assign
  window_capturer_win_gdi(const window_capturer_win_gdi &) = delete;
  window_capturer_win_gdi &operator=(const window_capturer_win_gdi &) = delete;

  ~window_capturer_win_gdi() override;

  static std::unique_ptr<desktop_capturer>
  create_raw_window_capturer(const desktop_capture_options &options);

  // desktop_capturer interface.
  uint32_t current_capturer_id() const override { return desktop_capture_id::k_capture_gdi_win; }
  void start(desktop_capturer::capture_callback *callback) override;
  void capture_frame() override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;
  bool focus_on_selected_source() override;
  bool is_occluded(const desktop_vector &pos) override;

private:
  struct capture_results {
    capture_result result;
    std::unique_ptr<desktop_frame> frame;
  };

  capture_results capture_frame(bool capture_owned_windows);

  capture_callback *callback_ = nullptr;

  // HWND and HDC for the currently selected window or nullptr if window is not
  // selected.
  HWND window_ = nullptr;

  desktop_size previous_size_;

  window_capture_helper_win window_capture_helper_;

  bool enumerate_current_process_windows_;

  // This map is used to avoid flickering for the case when SelectWindow() calls
  // are interleaved with Capture() calls.
  std::map<HWND, desktop_size> window_size_map_;

  window_finder_win window_finder_;

  std::vector<HWND> owned_windows_;
  std::unique_ptr<window_capturer_win_gdi> owned_window_capturer_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_WINDOW_CAPTURER_WIN_GDI_H_
