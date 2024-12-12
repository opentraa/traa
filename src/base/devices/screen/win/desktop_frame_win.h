/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_FRAME_WIN_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_FRAME_WIN_H_

#include "base/devices/screen/desktop_frame.h"

#include <windows.h>

#include <memory>

namespace traa {
namespace base {

// desktop_frame implementation used by screen and window captures on Windows.
// Frame data is stored in a GDI bitmap.
class desktop_frame_win : public desktop_frame {
public:
  ~desktop_frame_win() override;

  desktop_frame_win(const desktop_frame_win &) = delete;
  desktop_frame_win &operator=(const desktop_frame_win &) = delete;

  static std::unique_ptr<desktop_frame_win>
  create(desktop_size size, shared_memory_factory *memory_factory, HDC hdc);

  HBITMAP bitmap() { return bitmap_; }

private:
  desktop_frame_win(desktop_size size, int stride, uint8_t *data,
                    std::unique_ptr<shared_memory> memroy, HBITMAP bitmap);

  HBITMAP bitmap_;
  std::unique_ptr<shared_memory> owned_shared_memory_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DESKTOP_FRAME_WIN_H_
