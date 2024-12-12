/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_H_
#define TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_H_

#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/platform.h"

#include <memory>

#if defined(TRAA_OS_MAC) && !defined(TRAA_OS_IOS)
#include "base/devices/screen/darwin/desktop_configuration_monitor.h"
#endif

namespace traa {
namespace base {

#if defined(TRAA_ENABLE_X11)
class x11_atom_cache;
#endif

// An interface to return the id of the visible window under a certain point.
class window_finder {
public:
  window_finder() = default;
  virtual ~window_finder() = default;

  // Returns the id of the visible window under `point`. This function returns
  // kNullWindowId if no window is under `point` and the platform does not have
  // "root window" concept, i.e. the visible area under `point` is the desktop.
  // `point` is always in system coordinate, i.e. the primary monitor always
  // starts from (0, 0).
  virtual win_id_t get_window_under_point(desktop_vector point) = 0;

  struct options final {
    options() = default;
    ~options() = default;
    options(const options &other) = default;
    options(options &&other) = default;

#if defined(TRAA_ENABLE_X11)
    x11_atom_cache *cache = nullptr;
#endif
#if defined(TRAA_OS_MAC) && !defined(TRAA_OS_IOS)
    std::shared_ptr<desktop_configuration_monitor> configuration_monitor;
#endif
  };

  // Creates a platform-independent window_finder implementation. This function
  // returns nullptr if `options` does not contain enough information or
  // window_finder does not support current platform.
  static std::unique_ptr<window_finder> create(const options &options);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WINDOW_FINDER_H_
