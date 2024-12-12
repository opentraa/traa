/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_UTILS_WIN_GET_ACTIVATION_FACTORY_H_
#define TRAA_BASE_UTILS_WIN_GET_ACTIVATION_FACTORY_H_

#include <winerror.h>

#include "base/utils/win/hstring.h"

namespace traa {
namespace base {

// Provides access to Core WinRT functions which may not be available on
// Windows 7. Loads functions dynamically at runtime to prevent library
// dependencies.

// Callers must check the return value of ResolveCoreWinRTDelayLoad() before
// using these functions.

bool resolve_core_winrt_delayload();

HRESULT ro_get_activation_factory_proxy(HSTRING class_id, const IID &iid, void **out_factory);

// Retrieves an activation factory for the type specified.
template <typename InterfaceType, wchar_t const *runtime_class_id>
HRESULT get_activation_factory(InterfaceType **factory) {
  HSTRING class_id_hstring;
  HRESULT hr = create_hstring(runtime_class_id, static_cast<uint32_t>(wcslen(runtime_class_id)),
                              &class_id_hstring);
  if (FAILED(hr))
    return hr;

  hr = ro_get_activation_factory_proxy(class_id_hstring, IID_PPV_ARGS(factory));
  if (FAILED(hr)) {
    delete_hstring(class_id_hstring);
    return hr;
  }

  return delete_hstring(class_id_hstring);
}

} // namespace base
} // namespace traa

#endif // TRAA_BASE_UTILS_WIN_GET_ACTIVATION_FACTORY_H_