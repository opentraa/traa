/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/fake_desktop_capturer.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_frame.h"

#include <utility>

namespace traa {
namespace base {

fake_desktop_capturer::fake_desktop_capturer() = default;
fake_desktop_capturer::~fake_desktop_capturer() = default;

void fake_desktop_capturer::set_result(desktop_capturer::capture_result result) {
  result_ = result;
}

int fake_desktop_capturer::num_frames_captured() const {
  return num_frames_captured_;
}

int fake_desktop_capturer::num_capture_attempts() const {
  return num_capture_attempts_;
}

// Uses the `generator` provided as DesktopFrameGenerator, FakeDesktopCapturer
// does
// not take the ownership of `generator`.
void fake_desktop_capturer::set_frame_generator(
    desktop_frame_generator* generator) {
  generator_ = generator;
}

void fake_desktop_capturer::start(desktop_capturer::capture_callback* callback) {
  callback_ = callback;
}

void fake_desktop_capturer::capture_frame() {
  num_capture_attempts_++;
  if (generator_) {
    if (result_ != desktop_capturer::capture_result::success) {
      callback_->on_capture_result(result_, nullptr);
      return;
    }

    std::unique_ptr<desktop_frame> frame(
        generator_->get_next_frame(shared_memory_factory_.get()));
    if (frame) {
      num_frames_captured_++;
      callback_->on_capture_result(result_, std::move(frame));
    } else {
      callback_->on_capture_result(desktop_capturer::capture_result::error_temporary,
                                 nullptr);
    }
    return;
  }
  callback_->on_capture_result(desktop_capturer::capture_result::error_permanent, nullptr);
}

void fake_desktop_capturer::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> shared_memory_factory) {
  shared_memory_factory_ = std::move(shared_memory_factory);
}

bool fake_desktop_capturer::get_source_list(source_list_t* sources) {
  sources->push_back({k_window_id, "A-Fake-DesktopCapturer-Window"});
  sources->push_back({k_screen_id, "A-Fake-DesktopCapturer-Screen"});
  return true;
}

bool fake_desktop_capturer::select_source(source_id_t id) {
  return id == k_window_id || id == k_screen_id || id == k_screen_id_full;
}

} // namespace base
} // namespace traa
