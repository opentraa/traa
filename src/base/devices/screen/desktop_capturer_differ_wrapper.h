/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DESKTOP_CAPTURER_DIFFER_WRAPPER_H_
#define TRAA_BASE_DEVICES_SCREEN_DESKTOP_CAPTURER_DIFFER_WRAPPER_H_

#include <memory>
#if defined(TRAA_ENABLE_WAYLAND)
#include "base/devices/screen/desktop_capture_metadata.h"
#endif // defined(TRAA_ENABLE_WAYLAND)
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/shared_memory.h"

namespace traa {
namespace base {

// desktop_capturer wrapper that calculates updated_region() by comparing frames
// content. This class always expects the underlying desktop_capturer
// implementation returns a superset of updated regions in DestkopFrame. If a
// desktop_capturer implementation does not know the updated region, it should
// set updated_region() to full frame.
//
// This class marks entire frame as updated if the frame size or frame stride
// has been changed.
class desktop_capturer_differ_wrapper : public desktop_capturer,
                                        public desktop_capturer::capture_callback {
public:
  // Creates a desktop_capturer_differ_wrapper with a desktop_capturer
  // implementation, and takes its ownership.
  explicit desktop_capturer_differ_wrapper(std::unique_ptr<desktop_capturer> base_capturer);

  ~desktop_capturer_differ_wrapper() override;

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
#if defined(TRAA_ENABLE_WAYLAND)
  desktop_capture_metadata get_metadata() override;
#endif // defined(TRAA_ENABLE_WAYLAND)
private:
  // desktop_capturer::capture_callback interface.
  void on_capture_result(capture_result result, std::unique_ptr<desktop_frame> frame) override;

  const std::unique_ptr<desktop_capturer> base_capturer_;
  desktop_capturer::capture_callback *callback_;
  std::unique_ptr<shared_desktop_frame> last_frame_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DESKTOP_CAPTURER_DIFFER_WRAPPER_H_
