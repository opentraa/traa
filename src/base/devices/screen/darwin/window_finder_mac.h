/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_WINDOW_FINDER_MAC_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_WINDOW_FINDER_MAC_H_

#include "base/devices/screen/window_finder.h"

namespace traa {
namespace base {

class desktop_configuration_monitor;

// The implementation of WindowFinder for Mac OSX.
class window_finder_mac final : public window_finder {
public:
  explicit window_finder_mac(std::shared_ptr<desktop_configuration_monitor> configuration_monitor);
  ~window_finder_mac() override;

  // WindowFinder implementation.
  win_id_t get_window_under_point(desktop_vector point) override;

private:
  const std::shared_ptr<desktop_configuration_monitor> configuration_monitor_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_WINDOW_FINDER_MAC_H_