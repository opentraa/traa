/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/platform.h"

#include <cstring>
#include <utility>

#include "base/devices/screen/desktop_capturer_differ_wrapper.h"

#if defined(TRAA_OS_WINDOWS)
#include "base/devices/screen/win/cropping_window_capturer.h"
#include "base/devices/screen/win/wgc/wgc_capturer_win.h"
#endif // defined(TRAA_OS_WINDOWS)

#if defined(TRAA_ENABLE_WAYLAND)
#include "base/devices/screen/linux/capture_utils.h"
#include "base/devices/screen/linux/wayland/base_capturer_pipewire.h"
#endif // defined(TRAA_ENABLE_WAYLAND)

namespace traa {
namespace base {

desktop_capturer::~desktop_capturer() = default;

delegated_source_list_controller *desktop_capturer::get_delegated_source_list_controller() {
  return nullptr;
}

void desktop_capturer::set_shared_memory_factory(
    std::unique_ptr<shared_memory_factory> shared_memory_factory) {}

void desktop_capturer::set_excluded_window(win_id_t window) {}

bool desktop_capturer::get_source_list(source_list_t *sources) { return true; }

bool desktop_capturer::select_source(source_id_t id) { return false; }

bool desktop_capturer::focus_on_selected_source() { return false; }

bool desktop_capturer::is_occluded(const desktop_vector &pos) { return false; }

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_window_capturer(const desktop_capture_options &options) {
#if defined(TRAA_OS_WINDOWS)
  if (options.allow_wgc_window_capturer() &&
      wgc_capturer_win::is_wgc_supported(capture_type::window)) {
    return wgc_capturer_win::create_raw_window_capturer(options);
  }

  if (options.allow_cropping_window_capturer()) {
    return cropping_window_capturer::create_capturer(options);
  }
#endif // defined(TRAA_OS_WINDOWS)

  std::unique_ptr<desktop_capturer> capturer = create_raw_window_capturer(options);
  if (capturer && options.detect_updated_region()) {
    capturer.reset(new desktop_capturer_differ_wrapper(std::move(capturer)));
  }

  return capturer;
}

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_screen_capturer(const desktop_capture_options &options) {
#if defined(TRAA_OS_WINDOWS)
  if (options.allow_wgc_screen_capturer() &&
      wgc_capturer_win::is_wgc_supported(capture_type::screen)) {
    return wgc_capturer_win::create_raw_screen_capturer(options);
  }
#endif // defined(TRAA_OS_WINDOWS)

  std::unique_ptr<desktop_capturer> capturer = create_raw_screen_capturer(options);
  if (capturer && options.detect_updated_region()) {
    capturer.reset(new desktop_capturer_differ_wrapper(std::move(capturer)));
  }

  return capturer;
}

// static
std::unique_ptr<desktop_capturer>
desktop_capturer::create_generic_capturer(const desktop_capture_options &options) {
  std::unique_ptr<desktop_capturer> capturer;

#if defined(TRAA_ENABLE_WAYLAND)
  if (options.allow_pipewire() && capture_utils::is_running_under_wayland()) {
    capturer = std::make_unique<base_capturer_pipewire>(options, capture_type::any_content);
  }

  if (capturer && options.detect_updated_region()) {
    capturer.reset(new desktop_capturer_differ_wrapper(std::move(capturer)));
  }
#endif // defined(TRAA_ENABLE_WAYLAND)

  return capturer;
}

} // namespace base
} // namespace traa
