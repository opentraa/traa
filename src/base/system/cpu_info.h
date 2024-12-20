/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_SYSTEM_CPU_INFO_H_
#define TRAA_BASE_SYSTEM_CPU_INFO_H_

#include <stdint.h>

namespace traa {
namespace base {

class cpu_info {
public:
  static uint32_t detect_number_of_cores();

private:
  cpu_info() {}
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_SYSTEM_CPU_INFO_H_
