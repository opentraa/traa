/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_MAC_H_
#define TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_MAC_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/darwin/desktop_configuration_monitor.h"
#include "base/devices/screen/darwin/desktop_frame_provider.h"
#include "base/devices/screen/screen_capture_frame_queue.h"
#include "base/devices/screen/screen_capturer_helper.h"
#include "base/devices/screen/shared_desktop_frame.h"

#include <CoreGraphics/CoreGraphics.h>

#include <memory>
#include <vector>

namespace traa {
namespace base {

// A class to perform video frame capturing for mac.
class screen_capturer_mac final : public desktop_capturer {
 public:
  screen_capturer_mac(std::shared_ptr<desktop_configuration_monitor> desktop_config_monitor,
                    bool detect_updated_region,
                    bool allow_iosurface);
  ~screen_capturer_mac() override;

  screen_capturer_mac(const screen_capturer_mac&) = delete;
  screen_capturer_mac& operator=(const screen_capturer_mac&) = delete;

  // TODO(julien.isorce): Remove init() or make it private.
  bool init();

  // desktop_capturer interface.
  void start(capture_callback* callback) override;
  void capture_frame() override;
  void set_excluded_window(win_id_t window) override;
  bool get_source_list(source_list_t* screens) override;
  bool select_source(source_id_t id) override;

 private:
  // Returns false if the selected screen is no longer valid.
  bool cg_blit(const desktop_frame& frame, const desktop_region& region);

  // Called when the screen configuration is changed.
  void screen_configuration_changed();

  bool register_refresh_and_move_handlers();
  void unregister_refresh_and_move_handlers();

  void screen_refresh(CGDirectDisplayID display_id,
                     CGRectCount count,
                     const CGRect* rect_array,
                     desktop_vector display_origin,
                     IOSurfaceRef io_surface);
  void release_buffers();

  std::unique_ptr<desktop_frame> create_frame();

  const bool detect_updated_region_;

  capture_callback* callback_ = nullptr;

  // Queue of the frames buffers.
  screen_capture_frame_queue<shared_desktop_frame> queue_;

  // Current display configuration.
  desktop_configuration desktop_config_;

  // Currently selected display, or 0 if the full desktop is selected. On OS X
  // 10.6 and before, this is always 0.
  CGDirectDisplayID current_display_ = 0;

  // The physical pixel bounds of the current screen.
  desktop_rect screen_pixel_bounds_;

  // The dip to physical pixel scale of the current screen.
  float dip_to_pixel_scale_ = 1.0f;

  // A thread-safe list of invalid rectangles, and the size of the most
  // recently captured screen.
  screen_capturer_helper helper_;

  // Contains an invalid region from the previous capture.
  desktop_region last_invalid_region_;

  // Monitoring display reconfiguration.
  std::shared_ptr<desktop_configuration_monitor> desktop_config_monitor_;

  CGWindowID excluded_window_ = 0;

  // List of streams, one per screen.
  std::vector<CGDisplayStreamRef> display_streams_;

  // Container holding latest state of the snapshot per displays.
  desktop_frame_provider desktop_frame_provider_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DARWIN_SCREEN_CAPTURER_MAC_H_
