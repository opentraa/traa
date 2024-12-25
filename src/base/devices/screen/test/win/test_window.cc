/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/win/test_window.h"

namespace traa {
namespace base {

inline namespace {

const WCHAR k_window_class[] = L"DesktopCaptureTestWindowClass";
const int k_window_height = 200;
const int k_window_width = 300;

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  switch (msg) {
  case WM_PAINT:
    PAINTSTRUCT paint_struct;
    HDC hdc = ::BeginPaint(hwnd, &paint_struct);

    // Paint the window so the color is consistent and we can inspect the
    // pixels in tests and know what to expect.
    ::FillRect(hdc, &paint_struct.rcPaint,
               ::CreateSolidBrush(
                   RGB(k_test_window_r_value, k_test_window_g_value, k_test_window_b_value)));

    ::EndPaint(hwnd, &paint_struct);
  }
  return ::DefWindowProc(hwnd, msg, w_param, l_param);
}

} // namespace

window_info create_test_window(const WCHAR *window_title, const int height, const int width,
                               const LONG extended_styles) {
  window_info info;
  ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&window_proc), &info.window_instance);

  WNDCLASSEXW wcex;
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.hInstance = info.window_instance;
  wcex.lpfnWndProc = &window_proc;
  wcex.lpszClassName = k_window_class;
  info.window_class = ::RegisterClassExW(&wcex);

  // Use the default height and width if the caller did not supply the optional
  // height and width parameters, or if they supplied invalid values.
  int window_height = height <= 0 ? k_window_height : height;
  int window_width = width <= 0 ? k_window_width : width;
  info.hwnd = ::CreateWindowExW(extended_styles, k_window_class, window_title, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                                /*parent_window=*/nullptr,
                                /*menu_bar=*/nullptr, info.window_instance,
                                /*additional_params=*/nullptr);

  ::ShowWindow(info.hwnd, SW_SHOWNORMAL);
  ::UpdateWindow(info.hwnd);
  return info;
}

void resize_test_window(const HWND hwnd, const int width, const int height) {
  // SWP_NOMOVE results in the x and y params being ignored.
  ::SetWindowPos(hwnd, HWND_TOP, /*x-coord=*/0, /*y-coord=*/0, width, height,
                 SWP_SHOWWINDOW | SWP_NOMOVE);
  ::UpdateWindow(hwnd);
}

void move_test_window(const HWND hwnd, const int x, const int y) {
  // SWP_NOSIZE results in the width and height params being ignored.
  ::SetWindowPos(hwnd, HWND_TOP, x, y, /*width=*/0, /*height=*/0, SWP_SHOWWINDOW | SWP_NOSIZE);
  ::UpdateWindow(hwnd);
}

void minimize_test_window(const HWND hwnd) { ::ShowWindow(hwnd, SW_MINIMIZE); }

void unminimize_test_window(const HWND hwnd) { ::OpenIcon(hwnd); }

void destroy_test_window(window_info info) {
  ::DestroyWindow(info.hwnd);
  ::UnregisterClass(MAKEINTATOM(info.window_class), info.window_instance);
}

} // namespace base
} // namespace traa
