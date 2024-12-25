/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_CROPPING_WINDOW_CAPTURER_H_
#define TRAA_BASE_DEVICES_SCREEN_CROPPING_WINDOW_CAPTURER_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>

namespace traa {
namespace base {

// window capturer implementation that uses a screen capturer to capture the
// whole screen and crops the video frame to the window area when the captured
// window is on top.
class cropping_window_capturer : public desktop_capturer,
                                 public desktop_capturer::capture_callback {
public:
  static std::unique_ptr<desktop_capturer> create_capturer(const desktop_capture_options &options);

  ~cropping_window_capturer() override;

  // desktop_capturer implementation.
  void start(desktop_capturer::capture_callback *callback) override;
  void set_shared_memory_factory(std::unique_ptr<shared_memory_factory> memory_factory) override;
  void capture_frame() override;
  void set_excluded_window(win_id_t window) override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;
  bool focus_on_selected_source() override;
  bool is_occluded(const desktop_vector &pos) override;

  // desktop_capturer::capture_callback implementation, passed to `screen_capturer_` to
  // intercept the capture result.
  void on_capture_result(capture_result result, std::unique_ptr<desktop_frame> frame) override;

protected:
  explicit cropping_window_capturer(const desktop_capture_options &options);

  // The platform implementation should override these methods.

  // Returns true if it is OK to capture the whole screen and crop to the
  // selected window, i.e. the selected window is opaque, rectangular, and not
  // occluded.
  virtual bool should_use_screen_capturer() = 0;

  // Returns the window area relative to the top left of the virtual screen
  // within the bounds of the virtual screen. This function should return the
  // desktop_rect in full desktop coordinates, i.e. the top-left monitor starts
  // from (0, 0).
  virtual desktop_rect get_window_rect_in_virtual_screen() = 0;

  win_id_t selected_window() const { return selected_window_; }
  win_id_t excluded_window() const { return excluded_window_; }
  desktop_capturer *get_window_capturer() const { return window_capturer_.get(); }

private:
  desktop_capture_options options_;
  desktop_capturer::capture_callback *callback_;
  std::unique_ptr<desktop_capturer> window_capturer_;
  std::unique_ptr<desktop_capturer> screen_capturer_;
  win_id_t selected_window_;
  win_id_t excluded_window_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_CROPPING_WINDOW_CAPTURER_H_
