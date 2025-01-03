/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/window_finder.h"

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/test/screen_drawer.h"
#include "base/logger.h"

#include <gtest/gtest.h>

#include <stdint.h>

#include <memory>

#if defined(TRAA_OPTION_ENABLE_X11)
#include "base/devices/screen/linux/x11/shared_x_display.h"
#include "base/devices/screen/linux/x11/x_atom_cache.h"
#endif

#if defined(TRAA_OS_WINDOWS)
#include <windows.h>

#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/window_finder_win.h"
#endif

namespace traa {
namespace base {

namespace {

#if defined(TRAA_OS_WINDOWS)
// ScreenDrawerWin does not have a message loop, so it's unresponsive to user
// inputs. WindowFinderWin cannot detect this kind of unresponsive windows.
// Instead, console window is used to test WindowFinderWin.
// TODO(b/373792116): Reenable once flakiness is fixed.
TEST(window_finder_test, DISABLED_find_console_window) {
  // Creates a screen_drawer to avoid this test from conflicting with
  // ScreenCapturerIntegrationTest: both tests require its window to be in
  // foreground.
  //
  // In screen_capturer related tests, this is controlled by
  // screen_drawer, which has a global lock to ensure only one screen_drawer
  // window is active. So even we do not use screen_drawer for Windows test,
  // creating an instance can block screen_capturer related tests until this test
  // finishes.
  //
  // Usually the test framework should take care of this "isolated test"
  // requirement, but unfortunately WebRTC trybots do not support this.
  std::unique_ptr<screen_drawer> drawer = screen_drawer::create();
  const int k_max_size = 10000;
  // Enlarges current console window.
  system("mode 1000,1000");
  const HWND console_window = ::GetConsoleWindow();
  // Ensures that current console window is visible.
  ::ShowWindow(console_window, SW_MAXIMIZE);
  // Moves the window to the top-left of the display.
  if (!::MoveWindow(console_window, 0, 0, k_max_size, k_max_size, true)) {
    FAIL() << "Failed to move window. Error code: " << ::GetLastError();
  }

  bool should_restore_notopmost =
      (::GetWindowLong(console_window, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0;

  // Brings console window to top.
  if (!::SetWindowPos(console_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) {
    FAIL() << "Failed to bring window to top. Error code: " << ::GetLastError();
  }
  if (!::BringWindowToTop(console_window)) {
    FAIL() << "Failed second attempt to bring window to top. Error code: " << ::GetLastError();
  }

  bool success = false;
  window_finder_win finder;
  for (int i = 0; i < k_max_size; i++) {
    const desktop_vector spot(i, i);
    const HWND id = reinterpret_cast<HWND>(finder.get_window_under_point(spot));

    if (id == console_window) {
      success = true;
      break;
    }
    LOG_INFO("expected window {} . found window {}", reinterpret_cast<void *>(console_window),
             reinterpret_cast<void *>(id));
  }
  if (should_restore_notopmost)
    ::SetWindowPos(console_window, HWND_NOTOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

  if (!success)
    FAIL();
}

#else
TEST(window_finder_test, find_drawer_window) {
  window_finder::options options;
#if defined(TRAA_OPTION_ENABLE_X11)
  std::unique_ptr<x_atom_cache> cache;
  const auto x_display = shared_x_display::create_default();
  if (x_display) {
    cache = std::make_unique<x_atom_cache>(x_display->display());
    options.cache = cache.get();
  }
#endif
  std::unique_ptr<window_finder> finder = window_finder::create(options);
  if (!finder) {
    LOG_ERROR("No window_finder implementation for current platform.");
    return;
  }

  std::unique_ptr<screen_drawer> drawer = screen_drawer::create();
  if (!drawer) {
    LOG_ERROR("No screen_drawer implementation for current platform.");
    return;
  }

  if (drawer->window_id() == k_window_id_null) {
    // TODO(zijiehe): window_finder_test can use a dedicated window without
    // relying on screen_drawer.
    LOG_ERROR("screen_drawer implementation for current platform does "
              "create a window.");
    return;
  }

  // screen_drawer may not be able to bring the window to the top. So we test
  // several spots, at least one of them should succeed.
  const desktop_rect region = drawer->drawable_region();
  if (region.is_empty()) {
    LOG_ERROR("screen_drawer::drawable_region() is too small for the "
              "window_finder_test.");
    return;
  }

  for (int i = 0; i < region.width(); i++) {
    const desktop_vector spot(region.left() + i,
                              region.top() + i * region.height() / region.width());
    const win_id_t id = finder->get_window_under_point(spot);
    if (id == drawer->window_id()) {
      return;
    }
  }

  FAIL();
}
#endif

TEST(window_finder_test, should_return_null_window_if_spot_is_out_of_screen) {
  window_finder::options options;
#if defined(TRAA_OPTION_ENABLE_X11)
  std::unique_ptr<x_atom_cache> cache;
  const auto x_display = shared_x_display::create_default();
  if (x_display) {
    cache = std::make_unique<XAtomCache>(shared_x_display->display());
    options.cache = cache.get();
  }
#endif
  std::unique_ptr<window_finder> finder = window_finder::create(options);
  if (!finder) {
    LOG_ERROR("No window_finder implementation for current platform.");
    return;
  }

  ASSERT_EQ(k_window_id_null, finder->get_window_under_point(desktop_vector(INT16_MAX, INT16_MAX)));
  ASSERT_EQ(k_window_id_null, finder->get_window_under_point(desktop_vector(INT16_MAX, INT16_MIN)));
  ASSERT_EQ(k_window_id_null, finder->get_window_under_point(desktop_vector(INT16_MIN, INT16_MAX)));
  ASSERT_EQ(k_window_id_null, finder->get_window_under_point(desktop_vector(INT16_MIN, INT16_MIN)));
}

} // namespace

} // namespace base
} // namespace traa
