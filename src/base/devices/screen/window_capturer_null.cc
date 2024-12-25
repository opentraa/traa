/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"

namespace traa {
namespace base {

namespace {

class window_capturer_null : public desktop_capturer {
public:
  window_capturer_null();
  ~window_capturer_null() override;

  window_capturer_null(const window_capturer_null &) = delete;
  window_capturer_null &operator=(const window_capturer_null &) = delete;

  // desktop_capturer interface.
  void start(capture_callback *callback) override;
  void capture_frame() override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;

private:
  capture_callback *callback_ = nullptr;
};

window_capturer_null::window_capturer_null() {}
window_capturer_null::~window_capturer_null() {}

bool window_capturer_null::get_source_list(source_list_t *sources) {
  // Not implemented yet.
  return false;
}

bool window_capturer_null::select_source(source_id_t id) {
  // Not implemented yet.
  return false;
}

void window_capturer_null::start(capture_callback *callback) { callback_ = callback; }

void window_capturer_null::capture_frame() {
  // Not implemented yet.
  callback_->on_capture_result(capture_result::error_temporary, nullptr);
}

} // namespace

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_window_capturer(const desktop_capture_options &options) {
  return std::unique_ptr<desktop_capturer>(new window_capturer_null());
}

} // namespace base
} // namespace traa
