/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/blank_detector_desktop_capturer_wrapper.h"

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/logger.h"
#include "base/system/metrics.h"

#include <stdint.h>
#include <utility>

namespace traa {
namespace base {

blank_detector_desktop_capturer_wrapper::blank_detector_desktop_capturer_wrapper(
    std::unique_ptr<desktop_capturer> capturer, rgba_color blank_pixel, bool check_per_capture)
    : capturer_(std::move(capturer)), blank_pixel_(blank_pixel),
      check_per_capture_(check_per_capture) {}

blank_detector_desktop_capturer_wrapper::~blank_detector_desktop_capturer_wrapper() = default;

void blank_detector_desktop_capturer_wrapper::start(desktop_capturer::capture_callback *callback) {
  callback_ = callback;
  capturer_->start(this);
}

void blank_detector_desktop_capturer_wrapper::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> shared_memory_factory) {
  capturer_->set_shared_memory_factory(std::move(shared_memory_factory));
}

void blank_detector_desktop_capturer_wrapper::capture_frame() { capturer_->capture_frame(); }

void blank_detector_desktop_capturer_wrapper::set_excluded_window(win_id_t window) {
  capturer_->set_excluded_window(window);
}

bool blank_detector_desktop_capturer_wrapper::get_source_list(source_list_t *sources) {
  return capturer_->get_source_list(sources);
}

bool blank_detector_desktop_capturer_wrapper::select_source(source_id_t id) {
  if (check_per_capture_) {
    // If we start capturing a new source, we must reset these members
    // so we don't short circuit the blank detection logic.
    is_first_frame_ = true;
    non_blank_frame_received_ = false;
  }

  return capturer_->select_source(id);
}

bool blank_detector_desktop_capturer_wrapper::focus_on_selected_source() {
  return capturer_->focus_on_selected_source();
}

bool blank_detector_desktop_capturer_wrapper::is_occluded(const desktop_vector &pos) {
  return capturer_->is_occluded(pos);
}

void blank_detector_desktop_capturer_wrapper::on_capture_result(
    desktop_capturer::capture_result result, std::unique_ptr<desktop_frame> frame) {
  if (result != desktop_capturer::capture_result::success || non_blank_frame_received_) {
    callback_->on_capture_result(result, std::move(frame));
    return;
  }

  if (!frame) {
    // Capturer can call the blank detector with empty frame. Blank
    // detector regards it as a blank frame.
    callback_->on_capture_result(desktop_capturer::capture_result::error_temporary,
                                 std::unique_ptr<desktop_frame>());
    return;
  }

  // If nothing has been changed in current frame, we do not need to check it
  // again.
  if (!frame->updated_region().is_empty() || is_first_frame_) {
    last_frame_is_blank_ = is_blank_frame(*frame);
    is_first_frame_ = false;
  }
  TRAA_HISTOGRAM_BOOLEAN("WebRTC.DesktopCapture.BlankFrameDetected", last_frame_is_blank_);

  if (!last_frame_is_blank_) {
    non_blank_frame_received_ = true;
    callback_->on_capture_result(desktop_capturer::capture_result::success, std::move(frame));
    return;
  }

  callback_->on_capture_result(desktop_capturer::capture_result::error_temporary,
                               std::unique_ptr<desktop_frame>());
}

bool blank_detector_desktop_capturer_wrapper::is_blank_frame(const desktop_frame &frame) const {
  // We will check 7489 pixels for a frame with 1024 x 768 resolution.
  for (int i = 0; i < frame.size().width() * frame.size().height(); i += 105) {
    const int x = i % frame.size().width();
    const int y = i / frame.size().width();
    if (!is_blank_pixel(frame, x, y)) {
      return false;
    }
  }

  // We are verifying the pixel in the center as well.
  return is_blank_pixel(frame, frame.size().width() / 2, frame.size().height() / 2);
}

bool blank_detector_desktop_capturer_wrapper::is_blank_pixel(const desktop_frame &frame, int x,
                                                             int y) const {
  uint8_t *pixel_data = frame.get_frame_data_at_pos(desktop_vector(x, y));
  return rgba_color(pixel_data) == blank_pixel_;
}

} // namespace base
} // namespace traa
