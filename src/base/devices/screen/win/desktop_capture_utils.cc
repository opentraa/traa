/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/desktop_capture_utils.h"

#include <sstream>

namespace traa {
namespace base {

namespace desktop_capture_utils {

// Generates a human-readable string from a COM error.
std::string com_error_to_string(const _com_error &error) {
  std::stringstream ss;
  // Use _bstr_t to simplify the wchar to char conversion for ErrorMessage().
  _bstr_t error_message(error.ErrorMessage());
  ss << "HRESULT: 0x" << std::hex << error.Error()
     << ", message: " << static_cast<const char *>(error_message);
  return ss.str();
}

} // namespace desktop_capture_utils

} // namespace base
} // namespace traa
