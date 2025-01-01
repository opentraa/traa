/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/cropping_window_capturer.h"

#include "base/devices/screen/cropped_desktop_frame.h"
#include "base/logger.h"

#include <stddef.h>
#include <utility>

namespace traa {
namespace base {

cropping_window_capturer::cropping_window_capturer(const desktop_capture_options &options)
    : options_(options), callback_(NULL),
      window_capturer_(desktop_capturer::create_raw_window_capturer(options)),
      selected_window_(k_window_id_null), excluded_window_(k_window_id_null) {}

cropping_window_capturer::~cropping_window_capturer() {}

void cropping_window_capturer::start(desktop_capturer::capture_callback *callback) {
  callback_ = callback;
  window_capturer_->start(callback);
}

void cropping_window_capturer::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> memory_factory) {
  window_capturer_->set_shared_memory_factory(std::move(memory_factory));
}

void cropping_window_capturer::capture_frame() {
  if (should_use_screen_capturer()) {
    if (!screen_capturer_.get()) {
      screen_capturer_ = desktop_capturer::create_raw_screen_capturer(options_);
      if (excluded_window_) {
        screen_capturer_->set_excluded_window(excluded_window_);
      }
      screen_capturer_->start(this);
    }
    screen_capturer_->capture_frame();
  } else {
    window_capturer_->capture_frame();
  }
}

void cropping_window_capturer::set_excluded_window(win_id_t window) {
  excluded_window_ = window;
  if (screen_capturer_.get()) {
    screen_capturer_->set_excluded_window(window);
  }
}

bool cropping_window_capturer::get_source_list(source_list_t *sources) {
  return window_capturer_->get_source_list(sources);
}

bool cropping_window_capturer::select_source(source_id_t id) {
  if (window_capturer_->select_source(id)) {
    selected_window_ = id;
    return true;
  }
  return false;
}

bool cropping_window_capturer::focus_on_selected_source() {
  return window_capturer_->focus_on_selected_source();
}

void cropping_window_capturer::on_capture_result(desktop_capturer::capture_result result,
                                                 std::unique_ptr<desktop_frame> screen_frame) {
  if (!should_use_screen_capturer()) {
    LOG_INFO("window no longer on top when screen capturer finishes");
    window_capturer_->capture_frame();
    return;
  }

  if (result != capture_result::success) {
    LOG_WARN("screen capturer failed to capture a frame");
    callback_->on_capture_result(result, nullptr);
    return;
  }

  desktop_rect window_rect = get_window_rect_in_virtual_screen();
  if (window_rect.is_empty()) {
    LOG_WARN("window rect is empty");
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
    return;
  }

  std::unique_ptr<desktop_frame> cropped_frame =
      create_cropped_desktop_frame(std::move(screen_frame), window_rect);

  if (!cropped_frame) {
    LOG_WARN("window is outside of the captured display");
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
    return;
  }

  callback_->on_capture_result(capture_result::success, std::move(cropped_frame));
}

bool cropping_window_capturer::is_occluded(const desktop_vector &pos) {
  // Returns true if either capturer returns true.
  if (window_capturer_->is_occluded(pos)) {
    return true;
  }
  if (screen_capturer_ != nullptr && screen_capturer_->is_occluded(pos)) {
    return true;
  }
  return false;
}

} // namespace base
} // namespace traa
