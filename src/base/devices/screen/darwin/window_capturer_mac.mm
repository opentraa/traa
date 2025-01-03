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

#include "base/checks.h"
#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/darwin/desktop_configuration_monitor.h"
#include "base/devices/screen/darwin/desktop_frame_cgimage.h"
#include "base/devices/screen/darwin/window_finder_mac.h"
#include "base/devices/screen/darwin/window_list_utils.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/full_screen_window_detector.h"
#include "base/logger.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>

#include <utility>

namespace traa {
namespace base {

inline namespace {

// Returns true if the window exists.
bool is_window_valid(CGWindowID id) {
  CFArrayRef window_id_array =
      CFArrayCreate(nullptr, reinterpret_cast<const void **>(&id), 1, nullptr);
  CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
  bool valid = window_array && CFArrayGetCount(window_array);
  CFRelease(window_id_array);
  CFRelease(window_array);

  return valid;
}

class window_capturer_mac : public desktop_capturer {
public:
  explicit window_capturer_mac(
      std::shared_ptr<full_screen_window_detector> window_detector,
      std::shared_ptr<desktop_configuration_monitor> configuration_monitor);
  ~window_capturer_mac() override;

  window_capturer_mac(const window_capturer_mac &) = delete;
  window_capturer_mac &operator=(const window_capturer_mac &) = delete;

  // DesktopCapturer interface.
  void start(capture_callback *callback) override;
  void capture_frame() override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;
  bool focus_on_selected_source() override;
  bool is_occluded(const desktop_vector &pos) override;

private:
  capture_callback *callback_ = nullptr;

  // The window being captured.
  CGWindowID window_id_ = 0;

  std::shared_ptr<full_screen_window_detector> full_screen_window_detector_;

  const std::shared_ptr<desktop_configuration_monitor> configuration_monitor_;

  window_finder_mac window_finder_;

  // Used to make sure that we only log the usage of fullscreen detection once.
  bool fullscreen_usage_logged_ = false;
};

window_capturer_mac::window_capturer_mac(
    std::shared_ptr<full_screen_window_detector> window_detector,
    std::shared_ptr<desktop_configuration_monitor> configuration_monitor)
    : full_screen_window_detector_(std::move(window_detector)),
      configuration_monitor_(std::move(configuration_monitor)),
      window_finder_(configuration_monitor_) {}

window_capturer_mac::~window_capturer_mac() {}

bool window_capturer_mac::get_source_list(source_list_t *sources) {
  return get_window_list(sources, true, true);
}

bool window_capturer_mac::select_source(source_id_t id) {
  if (!is_window_valid(static_cast<CGWindowID>(id)))
    return false;
  window_id_ = static_cast<CGWindowID>(id);
  return true;
}

bool window_capturer_mac::focus_on_selected_source() {
  if (!window_id_)
    return false;

  CGWindowID ids[1];
  ids[0] = window_id_;
  CFArrayRef window_id_array =
      CFArrayCreate(nullptr, reinterpret_cast<const void **>(&ids), 1, nullptr);

  CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
  if (!window_array || 0 == CFArrayGetCount(window_array)) {
    // Could not find the window. It might have been closed.
    LOG_INFO("window not found");
    CFRelease(window_id_array);
    return false;
  }

  CFDictionaryRef window =
      reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, 0));
  CFNumberRef pid_ref =
      reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowOwnerPID));

  int pid;
  CFNumberGetValue(pid_ref, kCFNumberIntType, &pid);

  // TODO(jiayl): this will bring the process main window to the front. We
  // should find a way to bring only the window to the front.
  bool result =
      [[NSRunningApplication runningApplicationWithProcessIdentifier:pid] activateWithOptions:0];

  CFRelease(window_id_array);
  CFRelease(window_array);
  return result;
}

bool window_capturer_mac::is_occluded(const desktop_vector &pos) {
  desktop_vector sys_pos = pos;
  if (configuration_monitor_) {
    auto configuration = configuration_monitor_->get_desktop_configuration();
    sys_pos = pos.add(configuration.bounds.top_left());
  }
  return window_finder_.get_window_under_point(sys_pos) != window_id_;
}

void window_capturer_mac::start(capture_callback *callback) {
  TRAA_DCHECK(!callback_);
  TRAA_DCHECK(callback);

  callback_ = callback;
}

void window_capturer_mac::capture_frame() {
  LOG_INFO("window_capturer_mac::capture_frame");

  if (!is_window_valid(window_id_)) {
    LOG_ERROR("the window is not valid any longer.");
    callback_->on_capture_result(capture_result::error_permanent, nullptr);
    return;
  }

  CGWindowID on_screen_window = window_id_;
  if (full_screen_window_detector_) {
    full_screen_window_detector_->update_window_list_if_needed(
        window_id_, [](source_list_t *sources) {
          // Not using get_window_list(sources, true, false)
          // as it doesn't allow to have in the result window with
          // empty title along with titled window owned by the same pid.
          return get_window_list(
              [sources](CFDictionaryRef window) {
                win_id_t window_id = get_window_id(window);
                if (window_id != k_window_id_null) {
                  sources->push_back(source_t{window_id, get_window_title(window)});
                }
                return true;
              },
              true, false);
        });

    CGWindowID full_screen_window =
        static_cast<CGWindowID>(full_screen_window_detector_->find_full_screen_window(
            static_cast<source_id_t>(window_id_)));

    if (full_screen_window != kCGNullWindowID) {
      // If this is the first time this happens, report to UMA that the feature is active.
      if (!fullscreen_usage_logged_) {
        LOG_INFO("window_capturer_mac::capture_frame: full screen window detected");
        fullscreen_usage_logged_ = true;
      }
      on_screen_window = full_screen_window;
    }
  }

  std::unique_ptr<desktop_frame> frame = desktop_frame_cgimage::create_for_window(on_screen_window);
  if (!frame) {
    LOG_WARN("temporarily failed to capture window.");
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
    return;
  }

  frame->mutable_updated_region()->set_rect(desktop_rect::make_size(frame->size()));
  frame->set_top_left(get_window_bounds(on_screen_window).top_left());

  float scale_factor = get_window_scale_factor(window_id_, frame->size());
  frame->set_dpi(desktop_vector(desktop_frame::k_standard_dpi * scale_factor,
                                desktop_frame::k_standard_dpi * scale_factor));

  callback_->on_capture_result(capture_result::success, std::move(frame));
}

} // namespace

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_raw_window_capturer(const desktop_capture_options &options) {
  return std::unique_ptr<desktop_capturer>(new window_capturer_mac(
      options.get_full_screen_window_detector(), options.configuration_monitor()));
}

} // namespace base
} // namespace traa
