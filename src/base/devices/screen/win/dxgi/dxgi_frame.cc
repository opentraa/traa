/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_frame.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/win/dxgi/dxgi_duplicator_controller.h"
#include "base/logger.h"

#include <string.h>
#include <utility>

namespace traa {
namespace base {

dxgi_frame::dxgi_frame(shared_memory_factory *factory) : factory_(factory) {}

dxgi_frame::~dxgi_frame() = default;

bool dxgi_frame::prepare(desktop_size size, desktop_capturer::source_id_t source_id) {
  if (source_id != source_id_) {
    // Once the source has been changed, the entire source should be copied.
    source_id_ = source_id;
    context_.reset();
  }

  if (resolution_tracker_.set_resolution(size)) {
    // Once the output size changed, recreate the shared_desktop_frame.
    frame_.reset();
  }

  if (!frame_) {
    std::unique_ptr<desktop_frame> frame;
    if (factory_) {
      frame = shared_memory_desktop_frame::create(size, factory_);

      if (!frame) {
        LOG_WARN("dxgi_frame cannot create a new desktop_frame.");
        return false;
      }

      // DirectX capturer won't paint each pixel in the frame due to its one
      // capturer per monitor design. So once the new frame is created, we
      // should clear it to avoid the legacy image to be remained on it. See
      // http://crbug.com/708766.
      memset(frame->data(), 0, frame->stride() * frame->size().height());
    } else {
      frame.reset(new basic_desktop_frame(size));
    }

    frame_ = shared_desktop_frame::wrap(std::move(frame));
  }

  return !!frame_;
}

shared_desktop_frame *dxgi_frame::frame() const { return frame_.get(); }

dxgi_frame::context_t *dxgi_frame::get_context() { return &context_; }

} // namespace base
} // namespace traa
