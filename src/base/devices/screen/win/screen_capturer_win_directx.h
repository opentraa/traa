/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/screen_capture_frame_queue.h"
#include "base/devices/screen/win/dxgi/dxgi_duplicator_controller.h"
#include "base/devices/screen/win/dxgi/dxgi_frame.h"

#include <d3dcommon.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace traa {
namespace base {

// screen_capturer_win_directx captures 32bit RGBA using DirectX.
class screen_capturer_win_directx : public desktop_capturer {
public:
  using d3d_info = dxgi_duplicator_controller::d3d_info;

  // Whether the system supports DirectX based capturing.
  static bool is_supported();

  // Returns a most recent d3d_info composed by
  // dxgi_duplicator_controller::initialize() function. This function implicitly
  // calls dxgi_duplicator_controller::initialize() if it has not been
  // initialized. This function returns false and output parameter is kept
  // unchanged if dxgi_duplicator_controller::initialize() failed.
  // The d3d_info may change based on hardware configuration even without
  // restarting the hardware and software. Refer to https://goo.gl/OOCppq. So
  // consumers should not cache the result returned by this function.
  static bool retrieve_d3d_info(d3d_info *info);

  // Whether current process is running in a Windows session which is supported
  // by screen_capturer_win_directx.
  // Usually using screen_capturer_win_directx in unsupported sessions will fail.
  // But this behavior may vary on different Windows version. So consumers can
  // always try is_supported() function.
  static bool is_current_session_supported();

  // Maps `device_names` with the result from GetScreenList() and creates a new
  // SourceList to include only the ones in `device_names`. If this function
  // returns true, consumers can always assume `device_names`.size() equals to
  // `screens`->size(), meanwhile `device_names`[i] and `screens`[i] indicate
  // the same monitor on the system.
  // Public for test only.
  static bool get_screen_list_from_device_names(const std::vector<std::string> &device_names,
                                                desktop_capturer::source_list_t *screens);

  // Maps `id` with the result from get_screen_list_from_device_names() and returns
  // the index of the entity in `device_names`. This function returns -1 if `id`
  // cannot be found.
  // Public for test only.
  static int get_index_from_screen_id(desktop_capturer::source_id_t id,
                                      const std::vector<std::string> &device_names);

  // This constructor is deprecated. Please don't use it in new implementations.
  screen_capturer_win_directx();
  explicit screen_capturer_win_directx(const desktop_capture_options &options);

  ~screen_capturer_win_directx() override;

  screen_capturer_win_directx(const screen_capturer_win_directx &) = delete;
  screen_capturer_win_directx &operator=(const screen_capturer_win_directx &) = delete;

  // desktop_capturer implementation.
  uint32_t current_capturer_id() const override { return desktop_capture_id::k_capture_dxgi; }
  void start(capture_callback *callback) override;
  void set_shared_memory_factory(std::unique_ptr<shared_memory_factory> memory_factory) override;
  void capture_frame() override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;

private:
  const std::shared_ptr<dxgi_duplicator_controller> controller_;
  desktop_capture_options options_;

  // The underlying DxgiDuplicators may retain a reference to the frames that
  // we ask them to duplicate so that they can continue returning valid frames
  // in the event that the target has not been updated. Thus, we need to ensure
  // that we have a separate frame queue for each source id, so that these held
  // frames don't get overwritten with the data from another Duplicator/monitor.
  std::unordered_map<source_id_t, screen_capture_frame_queue<dxgi_frame>> frame_queue_map_;
  std::unique_ptr<shared_memory_factory> shared_memory_factory_;
  capture_callback *callback_ = nullptr;
  source_id_t current_screen_id_ = k_screen_id_full;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_
