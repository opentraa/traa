/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_frame_rotation.h"

#include <libyuv/rotate_argb.h>

namespace traa {
namespace base {

namespace {

libyuv::RotationMode to_libyuv_rotation_mode(rotation rot) {
  switch (rot) {
  case rotation::r_0:
    return libyuv::kRotate0;
  case rotation::r_90:
    return libyuv::kRotate90;
  case rotation::r_180:
    return libyuv::kRotate180;
  case rotation::r_270:
    return libyuv::kRotate270;
  }
  return libyuv::kRotate0;
}

desktop_rect rotate_and_offset_rect(desktop_rect rect, desktop_size size, rotation rot,
                                    desktop_vector offset) {
  desktop_rect result = rotate_rect(rect, size, rot);
  result.translate(offset);
  return result;
}

} // namespace

rotation reverse_rotation(rotation rot) {
  switch (rot) {
  case rotation::r_0:
    return rot;
  case rotation::r_90:
    return rotation::r_270;
  case rotation::r_180:
    return rotation::r_180;
  case rotation::r_270:
    return rotation::r_90;
  }
  return rotation::r_0;
}

desktop_size rotate_size(desktop_size size, rotation rot) {
  switch (rot) {
  case rotation::r_0:
  case rotation::r_180:
    return size;
  case rotation::r_90:
  case rotation::r_270:
    return desktop_size(size.height(), size.width());
  }

  return desktop_size();
}

desktop_rect rotate_rect(desktop_rect rect, desktop_size size, rotation rot) {
  switch (rot) {
  case rotation::r_0:
    return rect;
  case rotation::r_90:
    return desktop_rect::make_xywh(size.height() - rect.bottom(), rect.left(), rect.height(),
                                   rect.width());
  case rotation::r_180:
    return desktop_rect::make_xywh(size.width() - rect.right(), size.height() - rect.bottom(),
                                   rect.width(), rect.height());
  case rotation::r_270:
    return desktop_rect::make_xywh(rect.top(), size.width() - rect.right(), rect.height(),
                                   rect.width());
  }
  return desktop_rect();
}

void rotate_desktop_frame(const desktop_frame &source, const desktop_rect &source_rect,
                          const rotation &rot, const desktop_vector &target_offset,
                          desktop_frame *target) {
  // The rectangle in `target`.
  const desktop_rect target_rect =
      rotate_and_offset_rect(source_rect, source.size(), rot, target_offset);

  if (target_rect.is_empty()) {
    return;
  }

  libyuv::ARGBRotate(source.get_frame_data_at_pos(source_rect.top_left()), source.stride(),
                     target->get_frame_data_at_pos(target_rect.top_left()), target->stride(),
                     source_rect.width(), source_rect.height(), to_libyuv_rotation_mode(rot));
}

} // namespace base
} // namespace traa
