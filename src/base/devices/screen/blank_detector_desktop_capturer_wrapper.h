/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef BASE_DEVICES_SCREEN_BLANK_DETECTOR_DESKTOP_CAPTURER_WRAPPER_H_
#define BASE_DEVICES_SCREEN_BLANK_DETECTOR_DESKTOP_CAPTURER_WRAPPER_H_

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/rgba_color.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>

namespace traa {
namespace base {

// A desktop_capturer wrapper detects the return value of its owned
// desktop_capturer implementation. If sampled pixels returned by the
// desktop_capturer implementation all equal to the blank pixel, this wrapper
// returns ERROR_TEMPORARY. If the desktop_capturer implementation fails for too
// many times, this wrapper returns ERROR_PERMANENT.
class blank_detector_desktop_capturer_wrapper final : public desktop_capturer,
                                                      public desktop_capturer::capture_callback {
public:
  // Creates blank_detector_desktop_capturer_wrapper. blank_detector_desktop_capturer_wrapper
  // takes ownership of `capturer`. The `blank_pixel` is the unmodified color
  // returned by the `capturer`.
  blank_detector_desktop_capturer_wrapper(std::unique_ptr<desktop_capturer> capturer,
                                          rgba_color blank_pixel, bool check_per_capture = false);
  ~blank_detector_desktop_capturer_wrapper() override;

  // desktop_capturer interface.
  void start(desktop_capturer::capture_callback *callback) override;
  void
  set_shared_memory_factory(std::unique_ptr<shared_memory_factory> shared_memory_factory) override;
  void capture_frame() override;
  void set_excluded_window(win_id_t window) override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;
  bool focus_on_selected_source() override;
  bool is_occluded(const desktop_vector &pos) override;

private:
  // desktop_capturer::capture_callback interface.
  void on_capture_result(desktop_capturer::capture_result result,
                         std::unique_ptr<desktop_frame> frame) override;

  bool is_blank_frame(const desktop_frame &frame) const;

  // Detects whether pixel at (x, y) equals to `blank_pixel_`.
  bool is_blank_pixel(const desktop_frame &frame, int x, int y) const;

  const std::unique_ptr<desktop_capturer> capturer_;
  const rgba_color blank_pixel_;

  // Whether a non-blank frame has been received.
  bool non_blank_frame_received_ = false;

  // Whether the last frame is blank.
  bool last_frame_is_blank_ = false;

  // Whether current frame is the first frame.
  bool is_first_frame_ = true;

  // Blank inspection is made per capture instead of once for all
  // screens or windows.
  bool check_per_capture_ = false;

  desktop_capturer::capture_callback *callback_ = nullptr;
};

} // namespace base
} // namespace traa

#endif // BASE_DEVICES_SCREEN_BLANK_DETECTOR_DESKTOP_CAPTURER_WRAPPER_H_