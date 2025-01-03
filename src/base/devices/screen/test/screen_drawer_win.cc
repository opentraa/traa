/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/screen_drawer.h"

#include "base/system/sleep.h"

#include <windows.h>

#include <memory>

namespace traa {
namespace base {

namespace {

static constexpr TCHAR kMutexName[] =
    TEXT("Local\\screen_drawer_win-da834f82-8044-11e6-ac81-73dcdd1c1869");

class screen_drawer_lock_win : public screen_drawer_lock {
public:
  screen_drawer_lock_win();
  ~screen_drawer_lock_win() override;

private:
  HANDLE mutex_;
};

screen_drawer_lock_win::screen_drawer_lock_win() {
  while (true) {
    mutex_ = ::CreateMutex(NULL, FALSE, kMutexName);
    if (::GetLastError() != ERROR_ALREADY_EXISTS && mutex_ != NULL) {
      break;
    } else {
      if (mutex_) {
        ::CloseHandle(mutex_);
      }
      sleep_ms(1000);
    }
  }
}

screen_drawer_lock_win::~screen_drawer_lock_win() { ::CloseHandle(mutex_); }

desktop_rect get_screen_rect() {
  HDC hdc = ::GetDC(NULL);
  desktop_rect rect =
      desktop_rect::make_wh(::GetDeviceCaps(hdc, HORZRES), ::GetDeviceCaps(hdc, VERTRES));
  ::ReleaseDC(NULL, hdc);
  return rect;
}

HWND create_drawer_window(desktop_rect rect) {
  HWND hwnd = ::CreateWindowA("STATIC", "DrawerWindow", WS_POPUPWINDOW | WS_VISIBLE, rect.left(),
                              rect.top(), rect.width(), rect.height(), NULL, NULL, NULL, NULL);
  ::SetForegroundWindow(hwnd);
  return hwnd;
}

COLORREF color_to_ref(rgba_color color) {
  // Windows device context does not support alpha.
  return RGB(color.red, color.green, color.blue);
}

// A screen_drawer implementation for Windows.
class screen_drawer_win : public screen_drawer {
public:
  screen_drawer_win();
  ~screen_drawer_win() override;

  // screen_drawer interface.
  desktop_rect drawable_region() override;
  void draw_rectangle(desktop_rect rect, rgba_color color) override;
  void clear() override;
  void wait_for_pending_draws() override;
  bool may_draw_incomplete_shapes() override;
  win_id_t window_id() const override;

private:
  // Bring the window to the front, this can help to avoid the impact from other
  // windows or shadow effects.
  void bring_to_front();

  // Draw a line with `color`.
  void draw_line(desktop_vector start, desktop_vector end, rgba_color color);

  // Draw a dot with `color`.
  void draw_dot(desktop_vector vect, rgba_color color);

  const desktop_rect rect_;
  HWND window_;
  HDC hdc_;
};

screen_drawer_win::screen_drawer_win()
    : screen_drawer(), rect_(get_screen_rect()), window_(create_drawer_window(rect_)),
      hdc_(::GetWindowDC(window_)) {
  // We do not need to handle any messages for the `window_`, so disable Windows
  // from processing windows ghosting feature.
  ::DisableProcessWindowsGhosting();

  // Always use stock pen (DC_PEN) and brush (DC_BRUSH).
  ::SelectObject(hdc_, ::GetStockObject(DC_PEN));
  ::SelectObject(hdc_, ::GetStockObject(DC_BRUSH));
  bring_to_front();
}

screen_drawer_win::~screen_drawer_win() {
  ::ReleaseDC(NULL, hdc_);
  ::DestroyWindow(window_);
  // Unfortunately there is no EnableProcessWindowsGhosting() API.
}

desktop_rect screen_drawer_win::drawable_region() { return rect_; }

void screen_drawer_win::draw_rectangle(desktop_rect rect, rgba_color color) {
  if (rect.width() == 1 && rect.height() == 1) {
    // Rectangle function cannot draw a 1 pixel rectangle.
    draw_dot(rect.top_left(), color);
    return;
  }

  if (rect.width() == 1 || rect.height() == 1) {
    // Rectangle function cannot draw a 1 pixel rectangle.
    draw_line(rect.top_left(), desktop_vector(rect.right(), rect.bottom()), color);
    return;
  }

  ::SetDCBrushColor(hdc_, color_to_ref(color));
  ::SetDCPenColor(hdc_, color_to_ref(color));
  ::Rectangle(hdc_, rect.left(), rect.top(), rect.right(), rect.bottom());
}

void screen_drawer_win::clear() { draw_rectangle(rect_, rgba_color(0, 0, 0)); }

// TODO(zijiehe): Find the right signal to indicate the finish of all pending
// paintings.
void screen_drawer_win::wait_for_pending_draws() {
  bring_to_front();
  sleep_ms(50);
}

bool screen_drawer_win::may_draw_incomplete_shapes() { return true; }

win_id_t screen_drawer_win::window_id() const { return reinterpret_cast<win_id_t>(window_); }

void screen_drawer_win::draw_line(desktop_vector start, desktop_vector end, rgba_color color) {
  POINT points[2];
  points[0].x = start.x();
  points[0].y = start.y();
  points[1].x = end.x();
  points[1].y = end.y();
  ::SetDCPenColor(hdc_, color_to_ref(color));
  ::Polyline(hdc_, points, 2);
}

void screen_drawer_win::draw_dot(desktop_vector vect, rgba_color color) {
  ::SetPixel(hdc_, vect.x(), vect.y(), color_to_ref(color));
}

void screen_drawer_win::bring_to_front() {
  if (::SetWindowPos(window_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) != FALSE) {
    return;
  }

  long ex_style = ::GetWindowLong(window_, GWL_EXSTYLE);
  ex_style |= WS_EX_TOPMOST;
  if (::SetWindowLong(window_, GWL_EXSTYLE, ex_style) != 0) {
    return;
  }

  ::BringWindowToTop(window_);
}

} // namespace

// static
std::unique_ptr<screen_drawer_lock> screen_drawer_lock::create() {
  return std::unique_ptr<screen_drawer_lock>(new screen_drawer_lock_win());
}

// static
std::unique_ptr<screen_drawer> screen_drawer::create() {
  return std::unique_ptr<screen_drawer>(new screen_drawer_win());
}

} // namespace base
} // namespace traa