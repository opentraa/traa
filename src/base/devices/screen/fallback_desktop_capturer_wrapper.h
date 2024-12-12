/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_FALLBACK_DESKTOP_CAPTURER_WRAPPER_H_
#define TRAA_BASE_DEVICES_SCREEN_FALLBACK_DESKTOP_CAPTURER_WRAPPER_H_

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>

namespace traa {
namespace base {

// A desktop_capturer wrapper owns two desktop_capturer implementations. If the
// main desktop_capturer fails, it uses the secondary one instead. Two capturers
// are expected to return same SourceList, and the meaning of each SourceId is
// identical, otherwise fallback_desktop_capturer_wrapper may return frames from
// different sources. Using asynchronized desktop_capturer implementations with
// shared_memory_factory is not supported, and may result crash or assertion
// failure.
class fallback_desktop_capturer_wrapper final : public desktop_capturer,
                                                public desktop_capturer::capture_callback {
public:
  fallback_desktop_capturer_wrapper(std::unique_ptr<desktop_capturer> main_capturer,
                                    std::unique_ptr<desktop_capturer> secondary_capturer);
  ~fallback_desktop_capturer_wrapper() override;

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

  const std::unique_ptr<desktop_capturer> main_capturer_;
  const std::unique_ptr<desktop_capturer> secondary_capturer_;
  std::unique_ptr<shared_memory_factory> shared_memory_factory_;
  bool main_capturer_permanent_error_ = false;
  desktop_capturer::capture_callback *callback_ = nullptr;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_FALLBACK_DESKTOP_CAPTURER_WRAPPER_H_
