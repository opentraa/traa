/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/fallback_desktop_capturer_wrapper.h"

#include "base/logger.h"

#include <stddef.h>
#include <utility>

namespace traa {
namespace base {

inline namespace _impl {

// Implementation to share a shared_memory_factory between desktop_capturer
// instances. This class is designed for synchronized desktop_capturer
// implementations only.
class shared_memory_factory_proxy : public shared_memory_factory {
public:
  // Users should maintain the lifetime of `factory` to ensure it overlives
  // current instance.
  static std::unique_ptr<shared_memory_factory> create(shared_memory_factory *factory);
  ~shared_memory_factory_proxy() override;

  // Forwards create_shared_memory() calls to `factory_`. Users should always call
  // this function in one thread. Users should not call this function after the
  // shared_memory_factory which current instance created from has been destroyed.
  std::unique_ptr<shared_memory> create_shared_memory(size_t size) override;

private:
  explicit shared_memory_factory_proxy(shared_memory_factory *factory);

  shared_memory_factory *factory_ = nullptr;
};

shared_memory_factory_proxy::shared_memory_factory_proxy(shared_memory_factory *factory) {
  factory_ = factory;
}

// static
std::unique_ptr<shared_memory_factory>
shared_memory_factory_proxy::create(shared_memory_factory *factory) {
  return std::unique_ptr<shared_memory_factory>(new shared_memory_factory_proxy(factory));
}

shared_memory_factory_proxy::~shared_memory_factory_proxy() = default;

std::unique_ptr<shared_memory> shared_memory_factory_proxy::create_shared_memory(size_t size) {
  return factory_->create_shared_memory(size);
}

} // namespace _impl

fallback_desktop_capturer_wrapper::fallback_desktop_capturer_wrapper(
    std::unique_ptr<desktop_capturer> main_capturer,
    std::unique_ptr<desktop_capturer> secondary_capturer)
    : main_capturer_(std::move(main_capturer)), secondary_capturer_(std::move(secondary_capturer)) {
}

fallback_desktop_capturer_wrapper::~fallback_desktop_capturer_wrapper() = default;

void fallback_desktop_capturer_wrapper::start(desktop_capturer::capture_callback *callback) {
  callback_ = callback;
  // fallback_desktop_capturer_wrapper catchs the callback of the main capturer,
  // and checks its return value to decide whether the secondary capturer should
  // be involved.
  main_capturer_->start(this);
  // For the secondary capturer, we do not have a backup plan anymore, so
  // fallback_desktop_capturer_wrapper won't check its return value any more. It
  // will directly return to the input `callback`.
  secondary_capturer_->start(callback);
}

void fallback_desktop_capturer_wrapper::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> shared_memory_factory) {
  shared_memory_factory_ = std::move(shared_memory_factory);
  if (shared_memory_factory_) {
    main_capturer_->set_shared_memory_factory(
        shared_memory_factory_proxy::create(shared_memory_factory_.get()));
    secondary_capturer_->set_shared_memory_factory(
        shared_memory_factory_proxy::create(shared_memory_factory_.get()));
  } else {
    main_capturer_->set_shared_memory_factory(nullptr);
    secondary_capturer_->set_shared_memory_factory(nullptr);
  }
}

void fallback_desktop_capturer_wrapper::capture_frame() {
  if (main_capturer_permanent_error_) {
    secondary_capturer_->capture_frame();
  } else {
    main_capturer_->capture_frame();
  }
}

void fallback_desktop_capturer_wrapper::set_excluded_window(win_id_t window) {
  main_capturer_->set_excluded_window(window);
  secondary_capturer_->set_excluded_window(window);
}

bool fallback_desktop_capturer_wrapper::get_source_list(source_list_t *sources) {
  if (main_capturer_permanent_error_) {
    return secondary_capturer_->get_source_list(sources);
  }
  return main_capturer_->get_source_list(sources);
}

bool fallback_desktop_capturer_wrapper::select_source(source_id_t id) {
  if (main_capturer_permanent_error_) {
    return secondary_capturer_->select_source(id);
  }
  const bool main_capturer_result = main_capturer_->select_source(id);
  LOG_EVENT_COND("SDM", !main_capturer_result,
                 "fallback_desktop_capturer_wrapper::select_source failed");
  if (!main_capturer_result) {
    main_capturer_permanent_error_ = true;
  }

  return secondary_capturer_->select_source(id);
}

bool fallback_desktop_capturer_wrapper::focus_on_selected_source() {
  if (main_capturer_permanent_error_) {
    return secondary_capturer_->focus_on_selected_source();
  }
  return main_capturer_->focus_on_selected_source() ||
         secondary_capturer_->focus_on_selected_source();
}

bool fallback_desktop_capturer_wrapper::is_occluded(const desktop_vector &pos) {
  // Returns true if either capturer returns true.
  if (main_capturer_permanent_error_) {
    return secondary_capturer_->is_occluded(pos);
  }
  return main_capturer_->is_occluded(pos) || secondary_capturer_->is_occluded(pos);
}

void fallback_desktop_capturer_wrapper::on_capture_result(desktop_capturer::capture_result result,
                                                          std::unique_ptr<desktop_frame> frame) {
  LOG_EVENT_COND("SDM", result != desktop_capturer::capture_result::success,
                 "fallback_desktop_capturer_wrapper::on_capture_result success");
  LOG_EVENT_COND("SDM", result == desktop_capturer::capture_result::error_permanent,
                 "fallback_desktop_capturer_wrapper::on_capture_result error_permanent");
  if (result == desktop_capturer::capture_result::success) {
    callback_->on_capture_result(result, std::move(frame));
    return;
  }

  if (result == desktop_capturer::capture_result::error_permanent) {
    main_capturer_permanent_error_ = true;
  }
  secondary_capturer_->capture_frame();
}

} // namespace base
} // namespace traa
