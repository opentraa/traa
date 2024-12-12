/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_WIN_TEST_WINDOW_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_WIN_TEST_WINDOW_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace traa {
namespace base {

typedef unsigned char uint8_t;

// Define an arbitrary color for the test window with unique R, G, and B values
// so consumers can verify captured content in tests.
const uint8_t k_test_window_r_value = 191;
const uint8_t k_test_window_g_value = 99;
const uint8_t k_test_window_b_value = 12;

struct window_info {
  HWND hwnd;
  HINSTANCE window_instance;
  ATOM window_class;
};

window_info create_test_window(const WCHAR *window_title, int height = 0, int width = 0,
                               LONG extended_styles = 0);

void resize_test_window(HWND hwnd, int width, int height);

void move_test_window(HWND hwnd, int x, int y);

void minimize_test_window(HWND hwnd);

void unminimize_test_window(HWND hwnd);

void destroy_test_window(window_info info);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_WIN_TEST_WINDOW_H_
