/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/desktop_frame_win.h"

#include <utility>

#include "base/logger.h"

namespace traa {
namespace base {

desktop_frame_win::desktop_frame_win(desktop_size size, int stride, uint8_t *data,
                                     std::unique_ptr<shared_memory> memory, HBITMAP bitmap)
    : desktop_frame(size, stride, data, memory.get()), bitmap_(bitmap),
      owned_shared_memory_(std::move(memory)) {}

desktop_frame_win::~desktop_frame_win() { ::DeleteObject(bitmap_); }

// static
std::unique_ptr<desktop_frame_win>
desktop_frame_win::create(desktop_size size, shared_memory_factory *memory_factory, HDC hdc) {
  int bytes_per_row = size.width() * desktop_frame::k_bytes_per_pixel;
  int buffer_size = bytes_per_row * size.height();

  // Describe a device independent bitmap (DIB) that is the size of the desktop.
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biHeight = -size.height();
  bmi.bmiHeader.biWidth = size.width();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = desktop_frame::k_bytes_per_pixel * 8;
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biSizeImage = bytes_per_row * size.height();

  std::unique_ptr<shared_memory> memory;
  HANDLE section_handle = nullptr;
  if (memory_factory) {
    memory = memory_factory->create_shared_memory(buffer_size);
    if (!memory) {
      LOG_WARN("failed to allocate shared memory");
      return nullptr;
    }
    section_handle = memory->handle();
  }
  void *data = nullptr;
  HBITMAP bitmap = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &data, section_handle, 0);
  if (!bitmap) {
    LOG_WARN("failed to allocate new window frame {}", ::GetLastError());
    return nullptr;
  }

  return std::unique_ptr<desktop_frame_win>(new desktop_frame_win(
      size, bytes_per_row, reinterpret_cast<uint8_t *>(data), std::move(memory), bitmap));
}

} // namespace base
} // namespace traa
