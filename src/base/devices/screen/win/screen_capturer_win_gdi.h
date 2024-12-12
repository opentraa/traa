/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_GDI_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_GDI_H_

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/screen_capture_frame_queue.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/win/display_configuration_monitor.h"
#include "base/devices/screen/win/scoped_thread_desktop.h"

#include <windows.h>

#include <memory>

namespace traa {
namespace base {
// screen_capturer_win_gdi captures 32bit RGB using GDI.
//
// screen_capturer_win_gdi is double-buffered as required by desktop_capturer.
// This class does not detect desktop_frame::updated_region(), the field is
// always set to the entire frame rectangle. screen_capturer_differ_wrapper should
// be used if that functionality is necessary.
class screen_capturer_win_gdi : public desktop_capturer {
public:
  explicit screen_capturer_win_gdi(const desktop_capture_options &options);
  ~screen_capturer_win_gdi() override;

  screen_capturer_win_gdi(const screen_capturer_win_gdi &) = delete;
  screen_capturer_win_gdi &operator=(const screen_capturer_win_gdi &) = delete;

  // Overridden from desktop_capturer:
  uint32_t current_capturer_id() const override { return desktop_capture_id::k_capture_gdi_screen; }
  void start(capture_callback *callback) override;
  void set_shared_memory_factory(std::unique_ptr<shared_memory_factory> memory_factory) override;
  void capture_frame() override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;

private:
  bool disable_effects_ = false;

  // Make sure that the device contexts match the screen configuration.
  void prepare_capture_resources();

  // Captures the current screen contents into the current buffer. Returns true
  // if succeeded.
  bool capture_image();

  // Capture the current cursor shape.
  void capture_cursor();

  capture_callback *callback_ = nullptr;
  std::unique_ptr<shared_memory_factory> shared_memory_factory_;
  source_id_t current_screen_id_ = k_screen_id_full;
  std::wstring current_device_key_;

  scoped_thread_desktop desktop_;

  // GDI resources used for screen capture.
  HDC desktop_dc_ = NULL;
  HDC memory_dc_ = NULL;

  // Queue of the frames buffers.
  screen_capture_frame_queue<shared_desktop_frame> queue_;

  display_configuration_monitor display_configuration_monitor_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_GDI_H_
