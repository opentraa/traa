/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/shared_memory.h"

namespace traa {
namespace base {

#if defined(TRAA_OS_WINDOWS)
const shared_memory::NATIVE_HANDLE shared_memory::kInvalidHandle = NULL;
#else
const shared_memory::NATIVE_HANDLE shared_memory::kInvalidHandle = -1;
#endif

shared_memory::shared_memory(void *data, size_t size, NATIVE_HANDLE handle, int id)
    : data_(data), size_(size), handle_(handle), id_(id) {}

} // namespace base
} // namespace traa