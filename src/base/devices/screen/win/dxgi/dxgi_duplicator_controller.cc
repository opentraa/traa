/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_duplicator_controller.h"

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/dxgi/dxgi_frame.h"
#include "base/logger.h"
#include "base/system/sleep.h"
#include "base/utils/time_utils.h"

#include <windows.h>

#include <algorithm>
#include <string>

// TODO @sylar: replace this with :
// // Make sure we don't get min/max macros
// #ifndef NOMINMAX
// #define NOMINMAX
// #endif

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

namespace {

constexpr DWORD k_invalid_session_id = 0xFFFFFFFF;

DWORD get_current_session_id() {
  DWORD session_id = k_invalid_session_id;
  if (!::ProcessIdToSessionId(::GetCurrentProcessId(), &session_id)) {
    LOG_WARN(
        "Failed to retrieve current session Id, current binary may not have required priviledge.");
  }
  return session_id;
}

bool is_console_session() { return ::WTSGetActiveConsoleSessionId() == get_current_session_id(); }

} // namespace

// static
std::string
dxgi_duplicator_controller::result_name(dxgi_duplicator_controller::duplicate_result result) {
  switch (result) {
  case duplicate_result::succeeded:
    return "succeeded";
  case duplicate_result::unsupported_session:
    return "unsupported_session";
  case duplicate_result::frame_prepare_failed:
    return "frame_prepare_failed";
  case duplicate_result::initialization_failed:
    return "initialization_failed";
  case duplicate_result::duplication_failed:
    return "duplication_failed";
  case duplicate_result::invalid_monitor_id:
    return "invalid_monitor_id";
  default:
    return "unknown_error";
  }
}

// static
std::shared_ptr<dxgi_duplicator_controller> dxgi_duplicator_controller::instance() {
  static dxgi_duplicator_controller *_raw_instance = new dxgi_duplicator_controller();
  static std::weak_ptr<dxgi_duplicator_controller> _weak_instance;
  std::shared_ptr<dxgi_duplicator_controller> shared_instance = _weak_instance.lock();
  if (!shared_instance) {
    shared_instance = std::shared_ptr<dxgi_duplicator_controller>(
        _raw_instance, [](dxgi_duplicator_controller *p) {
          // only call unload
          p->unload();
        });
    _weak_instance = shared_instance;
  }
  return shared_instance;
}

// static
bool dxgi_duplicator_controller::is_current_session_supported() {
  DWORD current_session_id = get_current_session_id();
  return current_session_id != k_invalid_session_id && current_session_id != 0;
}

dxgi_duplicator_controller::dxgi_duplicator_controller() : refcount_(0) {}

bool dxgi_duplicator_controller::is_supported() {
  std::lock_guard<std::mutex> lock(mutex_);
  return initialize();
}

bool dxgi_duplicator_controller::retrieve_d3d_info(d3d_info *info) {
  bool result = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    result = initialize();
    *info = d3d_info_;
  }
  if (!result) {
    LOG_WARN("failed to initialize DXGI components, the D3dInfo retrieved may not accurate or out "
             "of date.");
  }
  return result;
}

dxgi_duplicator_controller::duplicate_result
dxgi_duplicator_controller::duplicate(dxgi_frame *frame) {
  return do_duplicate(frame, -1);
}

dxgi_duplicator_controller::duplicate_result
dxgi_duplicator_controller::duplicate_monitor(dxgi_frame *frame, int monitor_id) {
  return do_duplicate(frame, monitor_id);
}

desktop_vector dxgi_duplicator_controller::system_dpi() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialize()) {
    return system_dpi_;
  }
  return desktop_vector();
}

int dxgi_duplicator_controller::screen_count() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialize()) {
    return screen_count_unlocked();
  }
  return 0;
}

bool dxgi_duplicator_controller::get_device_names(std::vector<std::string> *output) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (initialize()) {
    get_device_names_unlocked(output);
    return true;
  }
  return false;
}

dxgi_duplicator_controller::duplicate_result
dxgi_duplicator_controller::do_duplicate(dxgi_frame *frame, int monitor_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  // The dxgi components and APIs do not update the screen resolution without
  // a reinitialization. So we use the GetDC() function to retrieve the screen
  // resolution to decide whether dxgi components need to be reinitialized.
  // If the screen resolution changed, it's very likely the next Duplicate()
  // function call will fail because of a missing monitor or the frame size is
  // not enough to store the output. So we reinitialize dxgi components in-place
  // to avoid a capture failure.
  // But there is no guarantee GetDC() function returns the same resolution as
  // dxgi APIs, we still rely on dxgi components to return the output frame
  // size.
  // TODO(zijiehe): Confirm whether IDXGIOutput::GetDesc() and
  // IDXGIOutputDuplication::GetDesc() can detect the resolution change without
  // reinitialization.
  if (display_configuration_monitor_.is_changed(frame->source_id_)) {
    deinitialize();
  }

  if (!initialize()) {
    if (succeeded_duplications_ == 0 && !is_current_session_supported()) {
      LOG_WARN("Current binary is running in session 0. DXGI components cannot be initialized.");
      return duplicate_result::unsupported_session;
    }

    // Cannot initialize COM components now, display mode may be changing.
    return duplicate_result::initialization_failed;
  }

  if (!frame->prepare(selected_desktop_size(monitor_id), monitor_id)) {
    return duplicate_result::frame_prepare_failed;
  }

  frame->frame()->mutable_updated_region()->clear();

  if (do_duplicate_unlocked(frame->get_context(), monitor_id, frame->frame())) {
    succeeded_duplications_++;
    return duplicate_result::succeeded;
  }
  if (monitor_id >= screen_count_unlocked()) {
    // It's a user error to provide a `monitor_id` larger than screen count. We
    // do not need to deinitialize.
    return duplicate_result::invalid_monitor_id;
  }

  // If the `monitor_id` is valid, but DoDuplicateUnlocked() failed, something
  // must be wrong from capturer APIs. We should Deinitialize().
  deinitialize();
  return duplicate_result::duplication_failed;
}

void dxgi_duplicator_controller::unload() {
  std::lock_guard<std::mutex> lock(mutex_);
  deinitialize();
}

void dxgi_duplicator_controller::unregister(const context_t *const context) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (context_expired(context)) {
    // The Context has not been setup after a recent initialization, so it
    // should not been registered in duplicators.
    return;
  }
  for (size_t i = 0; i < duplicators_.size(); i++) {
    duplicators_[i].unregister(&context->contexts[i]);
  }
}

bool dxgi_duplicator_controller::initialize() {
  if (!duplicators_.empty()) {
    return true;
  }

  if (do_initialize()) {
    return true;
  }
  deinitialize();
  return false;
}

bool dxgi_duplicator_controller::do_initialize() {
  d3d_info_.min_feature_level = static_cast<D3D_FEATURE_LEVEL>(0);
  d3d_info_.max_feature_level = static_cast<D3D_FEATURE_LEVEL>(0);

  std::vector<d3d_device> devices = d3d_device::enum_devices();
  if (devices.empty()) {
    LOG_WARN("no D3dDevice found.");
    return false;
  }

  for (size_t i = 0; i < devices.size(); i++) {
    D3D_FEATURE_LEVEL feature_level = devices[i].get_d3d_device()->GetFeatureLevel();
    if (d3d_info_.max_feature_level == 0 || feature_level > d3d_info_.max_feature_level) {
      d3d_info_.max_feature_level = feature_level;
    }
    if (d3d_info_.min_feature_level == 0 || feature_level < d3d_info_.min_feature_level) {
      d3d_info_.min_feature_level = feature_level;
    }

    dxgi_adapter_duplicator duplicator(devices[i]);
    // There may be several video cards on the system, some of them may not
    // support IDXGOutputDuplication. But they should not impact others from
    // taking effect, so we should continually try other adapters. This usually
    // happens when a non-official virtual adapter is installed on the system.
    if (!duplicator.initialize()) {
      LOG_WARN("Failed to initialize DxgiAdapterDuplicator on adapter ", i);
      continue;
    }

    duplicators_.push_back(std::move(duplicator));

    desktop_rect_.union_with(duplicators_.back().get_desktop_rect());
  }
  translate_rect();

  HDC hdc = ::GetDC(nullptr);
  // Use old DPI value if failed.
  if (hdc) {
    system_dpi_.set(::GetDeviceCaps(hdc, LOGPIXELSX), ::GetDeviceCaps(hdc, LOGPIXELSY));
    ::ReleaseDC(nullptr, hdc);
  }

  identity_++;

  if (duplicators_.empty()) {
    LOG_WARN("Cannot initialize any DxgiAdapterDuplicator instance.");
  }

  return !duplicators_.empty();
}

void dxgi_duplicator_controller::deinitialize() {
  desktop_rect_ = desktop_rect();
  duplicators_.clear();
  display_configuration_monitor_.reset();
}

bool dxgi_duplicator_controller::context_expired(const context_t *const context) const {
  return context->controller_id != identity_ || context->contexts.size() != duplicators_.size();
}

void dxgi_duplicator_controller::setup(context_t *context) {
  if (context_expired(context)) {
    context->contexts.clear();
    context->contexts.resize(duplicators_.size());
    for (size_t i = 0; i < duplicators_.size(); i++) {
      duplicators_[i].setup(&context->contexts[i]);
    }
    context->controller_id = identity_;
  }
}

bool dxgi_duplicator_controller::do_duplicate_unlocked(context_t *context, int monitor_id,
                                                       shared_desktop_frame *target) {
  setup(context);

  if (!ensure_frame_captured(context, target)) {
    return false;
  }

  bool result = false;
  if (monitor_id < 0) {
    // Capture entire screen.
    result = do_duplicate_all(context, target);
  } else {
    result = do_duplicate_one(context, monitor_id, target);
  }

  if (result) {
    target->set_dpi(system_dpi_);
    return true;
  }

  return false;
}

bool dxgi_duplicator_controller::do_duplicate_all(context_t *context,
                                                  shared_desktop_frame *target) {
  for (size_t i = 0; i < duplicators_.size(); i++) {
    if (!duplicators_[i].duplicate(&context->contexts[i], target)) {
      return false;
    }
  }
  return true;
}

bool dxgi_duplicator_controller::do_duplicate_one(context_t *context, int monitor_id,
                                                  shared_desktop_frame *target) {
  for (size_t i = 0; i < duplicators_.size() && i < context->contexts.size(); i++) {
    if (monitor_id >= duplicators_[i].screen_count()) {
      monitor_id -= duplicators_[i].screen_count();
    } else {
      if (duplicators_[i].duplicate_monitor(&context->contexts[i], monitor_id, target)) {
        target->set_top_left(duplicators_[i].get_screen_rect(monitor_id).top_left());
        return true;
      }
      return false;
    }
  }
  return false;
}

int64_t dxgi_duplicator_controller::get_num_frames_captured() const {
  int64_t min = INT64_MAX;
  for (const auto &duplicator : duplicators_) {
    min = std::min(min, duplicator.get_num_frames_captured());
  }

  return min;
}

desktop_size dxgi_duplicator_controller::get_desktop_size() const { return desktop_rect_.size(); }

desktop_rect dxgi_duplicator_controller::get_screen_rect(int id) const {
  for (size_t i = 0; i < duplicators_.size(); i++) {
    if (id >= duplicators_[i].screen_count()) {
      id -= duplicators_[i].screen_count();
    } else {
      return duplicators_[i].get_screen_rect(id);
    }
  }
  return desktop_rect();
}

int dxgi_duplicator_controller::screen_count_unlocked() const {
  int result = 0;
  for (auto &duplicator : duplicators_) {
    result += duplicator.screen_count();
  }
  return result;
}

void dxgi_duplicator_controller::get_device_names_unlocked(std::vector<std::string> *output) const {
  for (auto &duplicator : duplicators_) {
    for (int i = 0; i < duplicator.screen_count(); i++) {
      output->push_back(duplicator.get_device_name(i));
    }
  }
}

desktop_size dxgi_duplicator_controller::selected_desktop_size(int monitor_id) const {
  if (monitor_id < 0) {
    return get_desktop_size();
  }

  return get_screen_rect(monitor_id).size();
}

bool dxgi_duplicator_controller::ensure_frame_captured(context_t *context,
                                                       shared_desktop_frame *target) {
  // On a modern system, the FPS / monitor refresh rate is usually larger than
  // or equal to 60. So 17 milliseconds is enough to capture at least one frame.
  const int64_t ms_per_frame = 17;
  // Skip frames to ensure a full frame refresh has occurred and the DXGI
  // machinery is producing frames before this function returns.
  int64_t frames_to_skip = 1;
  // The total time out milliseconds for this function. If we cannot get enough
  // frames during this time interval, this function returns false, and cause
  // the DXGI components to be reinitialized. This usually should not happen
  // unless the system is switching display mode when this function is being
  // called. 500 milliseconds should be enough for ~30 frames.
  const int64_t timeout_ms = 500;

  if (get_num_frames_captured() == 0 && !is_console_session()) {
    // When capturing a console session, waiting for a single frame is
    // sufficient to ensure that DXGI output duplication is working. When the
    // session is not attached to the console, it has been observed that DXGI
    // may produce up to 4 frames (typically 1-2 though) before stopping. When
    // this condition occurs, no errors are returned from the output duplication
    // API, it simply appears that nothing is changing on the screen. Thus for
    // detached sessions, we need to capture a few extra frames before we can be
    // confident that output duplication was initialized properly.
    frames_to_skip = 5;
  }

  if (get_num_frames_captured() >= frames_to_skip) {
    return true;
  }

  std::unique_ptr<shared_desktop_frame> fallback_frame;
  shared_desktop_frame *shared_frame = nullptr;
  if (target->size().width() >= desktop_size().width() &&
      target->size().height() >= desktop_size().height()) {
    // `target` is large enough to cover entire screen, we do not need to use
    // `fallback_frame`.
    shared_frame = target;
  } else {
    fallback_frame = shared_desktop_frame::wrap(
        std::unique_ptr<desktop_frame>(new basic_desktop_frame(desktop_size())));
    shared_frame = fallback_frame.get();
  }

  const int64_t start_ms = time_millis();
  while (get_num_frames_captured() < frames_to_skip) {
    if (!do_duplicate_all(context, shared_frame)) {
      return false;
    }

    // Calling do_duplicate_all() may change the number of frames captured.
    if (get_num_frames_captured() >= frames_to_skip) {
      break;
    }

    if (time_millis() - start_ms > timeout_ms) {
      LOG_ERROR("Failed to capture {} frames within {} milliseconds.", frames_to_skip, timeout_ms);
      return false;
    }

    // Sleep `ms_per_frame` before attempting to capture the next frame to
    // ensure the video adapter has time to update the screen.
    sleep_ms(ms_per_frame);
  }
  // When capturing multiple monitors, we need to update the captured region to
  // prevent flickering by re-setting context. See
  // https://crbug.com/webrtc/15718 for details.
  if (shared_frame != target) {
    context->reset();
    setup(context);
  }
  return true;
}

void dxgi_duplicator_controller::translate_rect() {
  const desktop_vector position = desktop_vector().subtract(desktop_rect_.top_left());
  desktop_rect_.translate(position);
  for (auto &duplicator : duplicators_) {
    duplicator.translate_rect(position);
  }
}

} // namespace base
} // namespace traa
