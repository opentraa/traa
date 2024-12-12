/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURER_WIN_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURER_WIN_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/devices/screen/win/wgc/wgc_capture_session.h"
#include "base/devices/screen/win/wgc/wgc_capture_source.h"

#include <DispatcherQueue.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <map>
#include <memory>

namespace traa {
namespace base {

// wgc_capturer_win is initialized with an implementation of this base class,
// which it uses to find capturable sources of a particular type. This way,
// wgc_capturer_win can remain source-agnostic.
class source_enumerator {
public:
  virtual ~source_enumerator() = default;

  virtual bool find_all_sources(desktop_capturer::source_list_t *sources) = 0;
};

class window_enumerator final : public source_enumerator {
public:
  explicit window_enumerator(bool enumerate_current_process_windows)
      : enumerate_current_process_windows_(enumerate_current_process_windows) {}

  window_enumerator(const window_enumerator &) = delete;
  window_enumerator &operator=(const window_enumerator &) = delete;

  ~window_enumerator() override = default;

  bool find_all_sources(desktop_capturer::source_list_t *sources) override {
    // WGC fails to capture windows with the WS_EX_TOOLWINDOW style, so we
    // provide it as a filter to ensure windows with the style are not returned.
    return window_capture_helper_.enumerate_capturable_windows(
        sources, enumerate_current_process_windows_, WS_EX_TOOLWINDOW);
  }

private:
  window_capture_helper_win window_capture_helper_;
  bool enumerate_current_process_windows_;
};

class screen_enumerator final : public source_enumerator {
public:
  screen_enumerator() = default;

  screen_enumerator(const screen_enumerator &) = delete;
  screen_enumerator &operator=(const screen_enumerator &) = delete;

  ~screen_enumerator() override = default;

  bool find_all_sources(desktop_capturer::source_list_t *sources) override {
    return capture_utils::get_screen_list(sources);
  }
};

// A capturer that uses the Window.Graphics.Capture APIs. It is suitable for
// both window and screen capture (but only one type per instance). Consumers
// should not instantiate this class directly, instead they should use
// `CreateRawWindowCapturer()` or `CreateRawScreenCapturer()` to receive a
// capturer appropriate for the type of source they want to capture.
class wgc_capturer_win : public desktop_capturer {
public:
  wgc_capturer_win(const desktop_capture_options &options,
                   std::unique_ptr<wgc_capture_source_factory> source_factory,
                   std::unique_ptr<source_enumerator> enumerator,
                   bool allow_delayed_capturable_check);

  wgc_capturer_win(const wgc_capturer_win &) = delete;
  wgc_capturer_win &operator=(const wgc_capturer_win &) = delete;

  ~wgc_capturer_win() override;

  // Checks if the WGC API is present and supported on the system.
  static bool is_wgc_supported(capture_type type);

  static std::unique_ptr<desktop_capturer>
  create_raw_window_capturer(const desktop_capture_options &options,
                             bool allow_delayed_capturable_check = false);

  static std::unique_ptr<desktop_capturer>
  create_raw_screen_capturer(const desktop_capture_options &options);

  // desktop_capturer interface.
  uint32_t current_capturer_id() const override { return desktop_capture_id::k_capture_wgc; }
  bool get_source_list(desktop_capturer::source_list_t *sources) override;
  bool select_source(desktop_capturer::source_id_t id) override;
  bool focus_on_selected_source() override;
  void start(desktop_capturer::capture_callback *callback) override;
  void capture_frame() override;

  // Used in WgcCapturerTests.
  bool is_source_being_captured(desktop_capturer::source_id_t id);

private:
  typedef HRESULT(WINAPI *CreateDispatcherQueueControllerFunc)(
      DispatcherQueueOptions, ABI::Windows::System::IDispatcherQueueController **);

  desktop_capture_options options_;

  // We need to either create or ensure that someone else created a
  // `DispatcherQueue` on the current thread so that events will be delivered
  // on the current thread rather than an arbitrary thread. A
  // `DispatcherQueue`'s lifetime is tied to the thread's, and we don't post
  // any work to it, so we don't need to hold a reference.
  bool dispatcher_queue_created_ = false;

  // Statically linking to CoreMessaging.lib is disallowed in Chromium, so we
  // load it at runtime.
  HMODULE core_messaging_library_ = NULL;
  CreateDispatcherQueueControllerFunc create_dispatcher_queue_controller_func_ = nullptr;

  // Factory to create a wgc_capture_source for us whenever SelectSource is
  // called. Initialized at construction with a source-specific implementation.
  std::unique_ptr<wgc_capture_source_factory> source_factory_;

  // The source enumerator helps us find capturable sources of the appropriate
  // type. Initialized at construction with a source-specific implementation.
  std::unique_ptr<source_enumerator> source_enumerator_;

  // The wgc_capture_source represents the source we are capturing. It tells us
  // if the source is capturable and it creates the GraphicsCaptureItem for us.
  std::unique_ptr<wgc_capture_source> capture_source_;

  // A map of all the sources we are capturing and the associated
  // wgc_capture_session. Frames for the current source (indicated via
  // SelectSource) will be retrieved from the appropriate session when
  // requested via CaptureFrame.
  // This helps us efficiently capture multiple sources (e.g. when consumers
  // are trying to display a list of available capture targets with thumbnails).
  std::map<desktop_capturer::source_id_t, wgc_capture_session> ongoing_captures_;

  // The callback that we deliver frames to, synchronously, before CaptureFrame
  // returns.
  desktop_capturer::capture_callback *callback_ = nullptr;

  // wgc_capture_source::IsCapturable is expensive to run. So, caller can
  // delay capturable check till capture frame is called if the wgc_capturer_win
  // is used as a fallback capturer.
  bool allow_delayed_capturable_check_ = false;

  // A Direct3D11 device that is shared amongst the WgcCaptureSessions, who
  // require one to perform the capture.
  Microsoft::WRL::ComPtr<::ID3D11Device> d3d11_device_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURER_WIN_H_
