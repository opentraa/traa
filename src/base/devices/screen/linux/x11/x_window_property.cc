/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/linux/x11/x_window_property.h"

namespace traa {
namespace base {

x_window_property_base::x_window_property_base(Display *display, Window window, Atom property,
                                               int expected_size) {
  constexpr int k_bits_per_byte = 8;
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after; // NOLINT: type required by XGetWindowProperty
  int status = XGetWindowProperty(display, window, property, 0L, ~0L, False, AnyPropertyType,
                                  &actual_type, &actual_format, &size_, &bytes_after, &data_);
  if (status != Success) {
    data_ = nullptr;
    return;
  }
  if ((expected_size * k_bits_per_byte) != actual_format) {
    size_ = 0;
    return;
  }

  is_valid_ = true;
}

} // namespace base
} // namespace traa