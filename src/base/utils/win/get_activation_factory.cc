/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/utils/win/get_activation_factory.h"

#include <libloaderapi.h>
#include <roapi.h>

namespace {

FARPROC load_com_base_function(const char *function_name) {
  static HMODULE const handle =
      ::LoadLibraryExW(L"combase.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  return handle ? ::GetProcAddress(handle, function_name) : nullptr;
}

decltype(&::RoGetActivationFactory) get_ro_get_activation_factory_function() {
  static decltype(&::RoGetActivationFactory) const function =
      reinterpret_cast<decltype(&::RoGetActivationFactory)>(
          load_com_base_function("RoGetActivationFactory"));
  return function;
}

} // namespace

namespace traa {
namespace base {

bool resolve_core_winrt_delayload() {
  return get_ro_get_activation_factory_function() && resolve_core_winrt_string_delayload();
}

HRESULT ro_get_activation_factory_proxy(HSTRING class_id, const IID &iid, void **out_factory) {
  auto get_factory_func = get_ro_get_activation_factory_function();
  if (!get_factory_func)
    return E_FAIL;
  return get_factory_func(class_id, iid, out_factory);
}

} // namespace base
} // namespace traa
