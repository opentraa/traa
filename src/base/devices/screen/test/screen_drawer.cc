/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/screen_drawer.h"

namespace traa {
namespace base {

namespace {
std::unique_ptr<screen_drawer_lock> g_screen_drawer_lock;
} // namespace

screen_drawer_lock::screen_drawer_lock() = default;
screen_drawer_lock::~screen_drawer_lock() = default;

screen_drawer::screen_drawer() { g_screen_drawer_lock = screen_drawer_lock::create(); }

screen_drawer::~screen_drawer() { g_screen_drawer_lock.reset(); }

} // namespace base
} // namespace traa