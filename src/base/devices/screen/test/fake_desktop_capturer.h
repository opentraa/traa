/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_FAKE_DESKTOP_CAPTURER_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_FAKE_DESKTOP_CAPTURER_H_

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/test/desktop_frame_generator.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>

namespace traa {
namespace base {

// A fake implementation of DesktopCapturer or its derived interfaces to
// generate DesktopFrame for testing purpose.
//
// Consumers can provide a FrameGenerator instance to generate instances of
// DesktopFrame to return for each Capture() function call.
// If no FrameGenerator provided, FakeDesktopCapturer will always return a
// nullptr DesktopFrame.
//
// Double buffering is guaranteed by the FrameGenerator. FrameGenerator
// implements in desktop_frame_generator.h guarantee double buffering, they
// creates a new instance of DesktopFrame each time.
class fake_desktop_capturer : public desktop_capturer {
public:
  fake_desktop_capturer();
  ~fake_desktop_capturer() override;

  // Decides the result which will be returned in next Capture() callback.
  void set_result(desktop_capturer::capture_result result);

  // Uses the `generator` provided as DesktopFrameGenerator, FakeDesktopCapturer
  // does not take the ownership of `generator`.
  void set_frame_generator(desktop_frame_generator *generator);

  // Count of DesktopFrame(s) have been returned by this instance. This field
  // would never be negative.
  int num_frames_captured() const;

  // Count of CaptureFrame() calls have been made. This field would never be
  // negative.
  int num_capture_attempts() const;

  // DesktopCapturer interface
  void start(desktop_capturer::capture_callback *callback) override;
  void capture_frame() override;
  void
  set_shared_memory_factory(std::unique_ptr<shared_memory_factory> shared_memory_factory) override;
  bool get_source_list(source_list_t *sources) override;
  bool select_source(source_id_t id) override;

private:
  static constexpr source_id_t k_window_id = 1378277495;
  static constexpr source_id_t k_screen_id = 1378277496;

  desktop_capturer::capture_callback *callback_ = nullptr;
  std::unique_ptr<shared_memory_factory> shared_memory_factory_;
  desktop_capturer::capture_result result_ = desktop_capturer::capture_result::success;
  desktop_frame_generator *generator_ = nullptr;
  int num_frames_captured_ = 0;
  int num_capture_attempts_ = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_FAKE_DESKTOP_CAPTURER_H_
