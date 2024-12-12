/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_SHARED_DESKTOP_FRAME_H_
#define TRAA_BASE_DEVICES_SCREEN_SHARED_DESKTOP_FRAME_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "base/devices/screen/desktop_frame.h"

namespace traa {
namespace base {

// shared_desktop_frame is a desktop_frame that may have multiple instances all
// sharing the same buffer.
class shared_desktop_frame final : public desktop_frame {
public:
  ~shared_desktop_frame() override;

  shared_desktop_frame(const shared_desktop_frame &) = delete;
  shared_desktop_frame &operator=(const shared_desktop_frame &) = delete;

  static std::unique_ptr<shared_desktop_frame> wrap(std::unique_ptr<desktop_frame> frame);

  // Deprecated.
  // TODO(sergeyu): remove this method.
  static shared_desktop_frame *wrap(desktop_frame *frame);

  // Deprecated. Clients do not need to know the underlying desktop_frame
  // instance.
  // TODO(zijiehe): Remove this method.
  // Returns the underlying instance of desktop_frame.
  desktop_frame *get_underlying_frame();

  // Returns whether `this` and `other` share the underlying desktop_frame.
  bool share_frame_with(const shared_desktop_frame &other) const;

  // Creates a clone of this object.
  std::unique_ptr<shared_desktop_frame> share();

  // Checks if the frame is currently shared. If it returns false it's
  // guaranteed that there are no clones of the object.
  bool is_shared();

private:
  using shared_core_t = std::shared_ptr<std::unique_ptr<desktop_frame>>;

  shared_desktop_frame(shared_core_t core);

  const shared_core_t shared_core_;
};

} // namespace base
} // namespace traa

#endif // MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_
