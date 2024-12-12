/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/full_screen_window_detector.h"
#include "base/utils/time_utils.h"

namespace traa {
namespace base {

full_screen_window_detector::full_screen_window_detector(handler_factory_t handler_factory)
    : handler_factory_(handler_factory), last_update_time_ms_(0), previous_source_id_(0),
      no_handler_source_id_(0) {}

desktop_capturer::source_id_t full_screen_window_detector::find_full_screen_window(
    desktop_capturer::source_id_t original_source_id) {
  if (app_handler_ == nullptr || app_handler_->get_source_id() != original_source_id) {
    return 0;
  }
  return app_handler_->find_full_screen_window(window_list_, last_update_time_ms_);
}

void full_screen_window_detector::update_window_list_if_needed(
    desktop_capturer::source_id_t original_source_id,
    function_view<bool(desktop_capturer::source_list_t *)> get_sources) {
  const bool skip_update = previous_source_id_ != original_source_id;
  previous_source_id_ = original_source_id;

  // Here is an attempt to avoid redundant creating application handler in case
  // when an instance of WindowCapturer is used to generate a thumbnail to show
  // in picker by calling SelectSource and CaptureFrame for every available
  // source.
  if (skip_update) {
    return;
  }

  create_handler_if_needed(original_source_id);
  if (app_handler_ == nullptr) {
    // There is no  specific for
    // current application
    return;
  }

  constexpr int64_t k_update_interval_ms = 500;
  if ((time_millis() - last_update_time_ms_) <= k_update_interval_ms) {
    return;
  }

  desktop_capturer::source_list_t window_list;
  if (get_sources(&window_list)) {
    last_update_time_ms_ = time_millis();
    window_list_.swap(window_list);
  }
}

void full_screen_window_detector::create_handler_if_needed(
    desktop_capturer::source_id_t source_id_t) {
  if (no_handler_source_id_ == source_id_t) {
    return;
  }

  if (app_handler_ == nullptr || app_handler_->get_source_id() != source_id_t) {
    app_handler_ = handler_factory_ ? handler_factory_(source_id_t) : nullptr;
  }

  if (app_handler_ == nullptr) {
    no_handler_source_id_ = source_id_t;
  }
}

} // namespace base
} // namespace traa
