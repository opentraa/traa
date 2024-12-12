/* Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_MOCK_DESKTOP_CAPTURER_CALLBACK_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_MOCK_DESKTOP_CAPTURER_CALLBACK_H_

#include "base/devices/screen/desktop_capturer.h"

#include <gmock/gmock.h>

#include <memory>

namespace traa {
namespace base {

class mock_desktop_capturer_callback : public desktop_capturer::capture_callback {
public:
  mock_desktop_capturer_callback();
  ~mock_desktop_capturer_callback() override;

  mock_desktop_capturer_callback(const mock_desktop_capturer_callback &) = delete;
  mock_desktop_capturer_callback &operator=(const mock_desktop_capturer_callback &) = delete;

  MOCK_METHOD(void, on_capture_result_ptr,
              (desktop_capturer::capture_result result, std::unique_ptr<desktop_frame> *frame));
  void on_capture_result(desktop_capturer::capture_result result,
                         std::unique_ptr<desktop_frame> frame) override;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_MOCK_DESKTOP_CAPTURER_CALLBACK_H_
