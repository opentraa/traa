/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_FRAME_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_FRAME_H_

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/resolution_tracker.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/shared_memory.h"
#include "base/devices/screen/win/dxgi/dxgi_context.h"

#include <memory>
#include <vector>

namespace traa {
namespace base {

class dxgi_duplicator_controller;

// A pair of a SharedDesktopFrame and a dxgi_duplicator_controller::context_t for
// the client of dxgi_duplicator_controller.
class dxgi_frame final {
public:
  using context_t = dxgi_frame_context;

  // dxgi_frame does not take ownership of `factory`, consumers should ensure it
  // outlives this instance. nullptr is acceptable.
  explicit dxgi_frame(shared_memory_factory *factory);
  ~dxgi_frame();

  // Should not be called if prepare() is not executed or returns false.
  shared_desktop_frame *frame() const;

private:
  // Allows dxgi_duplicator_controller to access prepare() and context() function
  // as well as context_t class.
  friend class dxgi_duplicator_controller;

  // Prepares current instance with desktop size and source id.
  bool prepare(desktop_size size, desktop_capturer::source_id_t source_id);

  // Should not be called if prepare() is not executed or returns false.
  context_t *get_context();

  shared_memory_factory *const factory_;
  resolution_tracker resolution_tracker_;
  desktop_capturer::source_id_t source_id_ = k_screen_id_full;
  std::unique_ptr<shared_desktop_frame> frame_;
  context_t context_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_FRAME_H_
