/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/thread_desktop.h"

#include "base/logger.h"
#include "base/strings/string_trans.h"

#include <vector>

namespace traa {
namespace base {

thread_desktop::thread_desktop(HDESK desk, bool own) : desk_(desk), own_(own) {}

thread_desktop::~thread_desktop() {
  if (own_ && desk_ != NULL) {
    if (!::CloseDesktop(desk_)) {
      LOG_ERROR("Failed to close the owned thread_desktop handle: {}", ::GetLastError());
    }
  }
}

bool thread_desktop::get_name(std::wstring *desktop_name_out) const {
  if (desk_ == NULL)
    return false;

  DWORD length = 0;
  int rv = ::GetUserObjectInformationW(desk_, UOI_NAME, NULL, 0, &length);
  if (rv || ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    abort();

  length /= sizeof(WCHAR);
  std::vector<WCHAR> buffer(length);
  if (!::GetUserObjectInformationW(desk_, UOI_NAME, &buffer[0], length * sizeof(WCHAR), &length)) {
    LOG_ERROR("Failed to query the thread_desktop name: {}", ::GetLastError());
    return false;
  }

  desktop_name_out->assign(&buffer[0], length / sizeof(WCHAR));
  return true;
}

bool thread_desktop::is_same(const thread_desktop &other) const {
  std::wstring name;
  if (!get_name(&name))
    return false;

  std::wstring other_name;
  if (!other.get_name(&other_name))
    return false;

  return name == other_name;
}

bool thread_desktop::set_thread_desktop() const {
  if (!::SetThreadDesktop(desk_)) {
    LOG_ERROR("Failed to assign the thread_desktop to the current thread: {}", ::GetLastError());
    return false;
  }

  return true;
}

thread_desktop *thread_desktop::get_desktop(const WCHAR *desktop_name) {
  ACCESS_MASK desired_access = DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE |
                               DESKTOP_HOOKCONTROL | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                               DESKTOP_SWITCHDESKTOP | GENERIC_WRITE;
  HDESK desk = ::OpenDesktopW(desktop_name, 0, FALSE, desired_access);
  if (desk == NULL) {
    LOG_ERROR("failed to open the thread_desktop '{}': {}",
              string_trans::unicode_to_utf8(desktop_name), ::GetLastError());
    return nullptr;
  }

  return new thread_desktop(desk, true);
}

thread_desktop *thread_desktop::get_input_desktop() {
  HDESK desk = ::OpenInputDesktop(0, FALSE, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
  if (desk == NULL) {
    LOG_ERROR("Failed to open the input thread_desktop: {}", ::GetLastError());
    return nullptr;
  }

  return new thread_desktop(desk, true);
}

thread_desktop *thread_desktop::get_thread_desktop() {
  HDESK desk = ::GetThreadDesktop(GetCurrentThreadId());
  if (desk == NULL) {
    LOG_ERROR(
        "Failed to retrieve the handle of the thread_desktop assigned to the current thread: {}",
        ::GetLastError());
    return nullptr;
  }

  return new thread_desktop(desk, false);
}

} // namespace base
} // namespace traa
