/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/utils/win/hstring.h"

#include <libloaderapi.h>
#include <winstring.h>

namespace {

FARPROC load_com_base_function(const char *function_name) {
  static HMODULE const handle =
      ::LoadLibraryExW(L"combase.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  return handle ? ::GetProcAddress(handle, function_name) : nullptr;
}

decltype(&::WindowsCreateString) GetWindowsCreateString() {
  static decltype(&::WindowsCreateString) const function =
      reinterpret_cast<decltype(&::WindowsCreateString)>(
          load_com_base_function("WindowsCreateString"));
  return function;
}

decltype(&::WindowsDeleteString) GetWindowsDeleteString() {
  static decltype(&::WindowsDeleteString) const function =
      reinterpret_cast<decltype(&::WindowsDeleteString)>(
          load_com_base_function("WindowsDeleteString"));
  return function;
}

} // namespace

namespace traa {
namespace base {

bool resolve_core_winrt_string_delayload() {
  return ::GetWindowsDeleteString() && ::GetWindowsCreateString();
}

HRESULT create_hstring(const wchar_t *src, uint32_t len, HSTRING *out_hstr) {
  decltype(&::WindowsCreateString) create_string_func = ::GetWindowsCreateString();
  if (!create_string_func)
    return E_FAIL;
  return create_string_func(src, len, out_hstr);
}

HRESULT delete_hstring(HSTRING hstr) {
  decltype(&::WindowsDeleteString) delete_string_func = ::GetWindowsDeleteString();
  if (!delete_string_func)
    return E_FAIL;
  return delete_string_func(hstr);
}

} // namespace base
} // namespace traa
