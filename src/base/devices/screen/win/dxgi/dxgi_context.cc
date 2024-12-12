/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_context.h"

#include "base/devices/screen/win/dxgi/dxgi_duplicator_controller.h"

namespace traa {
namespace base {

dxgi_adapter_context::dxgi_adapter_context() = default;
dxgi_adapter_context::dxgi_adapter_context(const dxgi_adapter_context &context) = default;
dxgi_adapter_context::~dxgi_adapter_context() = default;

dxgi_frame_context::dxgi_frame_context() = default;

dxgi_frame_context::~dxgi_frame_context() { reset(); }

void dxgi_frame_context::reset() {
  dxgi_duplicator_controller::instance()->unregister(this);
  controller_id = 0;
}

} // namespace base
} // namespace traa
