/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_UTILS_WIN_HSTRING_H_
#define TRAA_BASE_UTILS_WIN_HSTRING_H_

#include <hstring.h>
#include <stdint.h>
#include <winerror.h>

namespace traa {
namespace base {

// Callers must check the return value of ResolveCoreWinRTStringDelayLoad()
// before using these functions.
bool resolve_core_winrt_string_delayload();

HRESULT create_hstring(const wchar_t *src, uint32_t len, HSTRING *out_hstr);

HRESULT delete_hstring(HSTRING hstr);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_UTILS_WIN_HSTRING_H_