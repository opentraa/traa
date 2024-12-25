/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/screen_capturer_win_directx.h"

#include "base/devices/screen/desktop_capturer.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

namespace traa {
namespace base {

// This test cannot ensure GetScreenListFromDeviceNames() won't reorder the
// devices in its output, since the device name is missing.
TEST(screen_capturer_win_directx_test, get_screen_list_from_device_names_and_get_index) {
  const std::vector<std::string> device_names = {
      "\\\\.\\DISPLAY0",
      "\\\\.\\DISPLAY1",
      "\\\\.\\DISPLAY2",
  };
  desktop_capturer::source_list_t screens;
  ASSERT_TRUE(
      screen_capturer_win_directx::get_screen_list_from_device_names(device_names, &screens));
  ASSERT_EQ(device_names.size(), screens.size());

  for (size_t i = 0; i < screens.size(); i++) {
    ASSERT_EQ(screen_capturer_win_directx::get_index_from_screen_id(screens[i].id, device_names),
              static_cast<int>(i));
  }
}

} // namespace base
} // namespace traa
