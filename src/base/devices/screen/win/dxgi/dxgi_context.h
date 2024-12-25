/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_CONTEXT_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_CONTEXT_H_

#include "base/devices/screen/desktop_region.h"

#include <vector>

namespace traa {
namespace base {

// A dxgi_output_context stores the status of a single dxgi_frame of
// dxgi_output_duplicator.
struct dxgi_output_context final {
  // The updated region dxgi_output_duplicator::detect_updated_region() output
  // during last duplicate() function call. It's always relative to the (0, 0).
  desktop_region updated_region;
};

// A dxgi_adapter_context stores the status of a single dxgi_frame of
// dxgi_adapter_duplicator.
struct dxgi_adapter_context final {
  dxgi_adapter_context();
  dxgi_adapter_context(const dxgi_adapter_context &other);
  ~dxgi_adapter_context();

  // Child DxgiOutputContext belongs to this AdapterContext.
  std::vector<dxgi_output_context> contexts;
};

// A dxgi_frame_context stores the status of a single dxgi_frame of
// dxgi_duplicator_controller.
struct dxgi_frame_context final {
public:
  dxgi_frame_context();
  // Unregister this Context instance from dxgi_duplicator_controller during
  // destructing.
  ~dxgi_frame_context();

  // Reset current Context, so it will be reinitialized next time.
  void reset();

  // A Context will have an exactly same `controller_id` as
  // dxgi_duplicator_controller, to ensure it has been correctly setted up after
  // each dxgi_duplicator_controller::Initialize().
  int controller_id = 0;

  // Child dxgi_adapter_context belongs to this dxgi_frame_context.
  std::vector<dxgi_adapter_context> contexts;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_CONTEXT_H_
