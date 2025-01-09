/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/cursor.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/mouse_cursor.h"
#include "base/devices/screen/test/win/cursor_unittest_resources.h"
#include "base/devices/screen/win/scoped_gdi_object.h"
#include <gtest/gtest.h>

#include <memory>

namespace traa {
namespace base {

namespace {

// Loads `left` from resources, converts it to a `mouse_cursor` instance and
// compares pixels with `right`. Returns true of mouse_cursor bits match `right`.
// `right` must be a 32bpp cursor with alpha channel.
bool convert_to_mouse_shape_and_compare(unsigned left, unsigned right) {
  HMODULE instance = ::GetModuleHandle(NULL);

  // Load `left` from the EXE module's resources.
  scoped_cursor_t cursor(reinterpret_cast<HCURSOR>(
      ::LoadImage(instance, MAKEINTRESOURCE(left), IMAGE_CURSOR, 0, 0, 0)));
  EXPECT_TRUE(cursor != NULL);

  // Convert `cursor` to `mouse_shape`.
  HDC dc = ::GetDC(NULL);
  std::unique_ptr<mouse_cursor> mouse_shape(create_mouse_cursor_from_handle(dc, cursor));
  ::ReleaseDC(NULL, dc);

  EXPECT_TRUE(mouse_shape.get());

  // Load `right`.
  cursor.set(reinterpret_cast<HCURSOR>(
      ::LoadImage(instance, MAKEINTRESOURCE(right), IMAGE_CURSOR, 0, 0, 0)));

  ICONINFO iinfo;
  EXPECT_TRUE(::GetIconInfo(cursor, &iinfo));
  EXPECT_TRUE(iinfo.hbmColor);

  // Make sure the bitmaps will be freed.
  scoped_bitmap_t scoped_mask(iinfo.hbmMask);
  scoped_bitmap_t scoped_color(iinfo.hbmColor);

  // Get `scoped_color` dimensions.
  BITMAP bitmap_info;
  EXPECT_TRUE(::GetObject(scoped_color, sizeof(bitmap_info), &bitmap_info));

  int width = bitmap_info.bmWidth;
  int height = bitmap_info.bmHeight;
  EXPECT_TRUE(desktop_size(width, height).equals(mouse_shape->image()->size()));

  // Get the pixels from `scoped_color`.
  int size = width * height;
  std::unique_ptr<uint32_t[]> data(new uint32_t[size]);
  EXPECT_TRUE(::GetBitmapBits(scoped_color, size * sizeof(uint32_t), data.get()));

  // Compare the 32bpp image in `mouse_shape` with the one loaded from `right`.
  return memcmp(data.get(), mouse_shape->image()->data(), size * sizeof(uint32_t)) == 0;
}

} // namespace

// TODO @sylar: why this test is disabled?
// call alpha_mul in create_mouse_cursor_from_handle will always make the final data in cursor_shape
// to all black value, seems like the we should not use RGBReserved, instead, we may should use
// ARGB value to do the alpha blending.
TEST(mouse_cursor_test, DISABLED_match_cursors) {
  EXPECT_TRUE(convert_to_mouse_shape_and_compare(IDD_CURSOR1_24BPP, IDD_CURSOR1_32BPP));

  EXPECT_TRUE(convert_to_mouse_shape_and_compare(IDD_CURSOR1_8BPP, IDD_CURSOR1_32BPP));

  EXPECT_TRUE(convert_to_mouse_shape_and_compare(IDD_CURSOR2_1BPP, IDD_CURSOR2_32BPP));

  EXPECT_TRUE(convert_to_mouse_shape_and_compare(IDD_CURSOR3_4BPP, IDD_CURSOR3_32BPP));
}

} // namespace base
} // namespace traa
