/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_ADAPTER_DUPLICATOR_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_ADAPTER_DUPLICATOR_H_

#include <wrl/client.h>

#include <vector>

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/win/d3d_device.h"
#include "base/devices/screen/win/dxgi/dxgi_context.h"
#include "base/devices/screen/win/dxgi/dxgi_output_duplicator.h"

namespace traa {
namespace base {

// A container of dxgi_output_duplicators to duplicate monitors attached to a
// single video card.
class dxgi_adapter_duplicator {
public:
  using context_t = dxgi_adapter_context;

  // Creates an instance of dxgi_adapter_duplicator from a D3dDevice. Only
  // dxgi_duplicator_controller can create an instance.
  explicit dxgi_adapter_duplicator(const d3d_device &device);

  // Move constructor, to make it possible to store instances of
  // dxgi_adapter_duplicator in std::vector<>.
  dxgi_adapter_duplicator(dxgi_adapter_duplicator &&other);

  ~dxgi_adapter_duplicator();

  // Initializes the dxgi_adapter_duplicator from a D3dDevice.
  bool initialize();

  // Sequentially calls duplicate function of all the dxgi_output_duplicator
  // instances owned by this instance, and writes into `target`.
  bool duplicate(context_t *context, shared_desktop_frame *target);

  // Captures one monitor and writes into `target`. `monitor_id` should be
  // between [0, screen_count()).
  bool duplicate_monitor(context_t *context, int monitor_id, shared_desktop_frame *target);

  // Returns desktop rect covered by this dxgi_adapter_duplicator.
  desktop_rect get_desktop_rect() const { return desktop_rect_; }

  // Returns the size of one screen owned by this dxgi_adapter_duplicator. `id`
  // should be between [0, screen_count()).
  desktop_rect get_screen_rect(int id) const;

  // Returns the device name of one screen owned by this dxgi_adapter_duplicator
  // in utf8 encoding. `id` should be between [0, screen_count()).
  const std::string &get_device_name(int id) const;

  // Returns the count of screens owned by this dxgi_adapter_duplicator. These
  // screens can be retrieved by an interger in the range of
  // [0, screen_count()).
  int screen_count() const;

  void setup(context_t *context);

  void unregister(const context_t *const context);

  // The minimum num_frames_captured() returned by `duplicators_`.
  int64_t get_num_frames_captured() const;

  // Moves `desktop_rect_` and all underlying `duplicators_`. See
  // dxgi_duplicator_controller::translate_rect().
  void translate_rect(const desktop_vector &position);

private:
  bool do_initialize();

  const d3d_device device_;
  std::vector<dxgi_output_duplicator> duplicators_;
  desktop_rect desktop_rect_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_ADAPTER_DUPLICATOR_H_