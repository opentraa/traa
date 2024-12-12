/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_

#include <windows.h>

namespace traa {
namespace base {

class mouse_cursor;

// Converts an HCURSOR into a `mouse_cursor` instance.
mouse_cursor *create_mouse_cursor_from_handle(HDC dc, HCURSOR cursor);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_