/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_LOCK_POSIX_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_LOCK_POSIX_H_

#include "base/devices/screen/test/screen_drawer.h"

#include <semaphore.h>
#include <string_view>

namespace traa {
namespace base {

class screen_drawer_lock_posix final : public screen_drawer_lock {
public:
  screen_drawer_lock_posix();
  // Provides a name other than the default one for test only.
  explicit screen_drawer_lock_posix(const char *name);
  ~screen_drawer_lock_posix() override;

  // Unlinks the named semaphore actively. This will remove the sem_t object in
  // the system and allow others to create a different sem_t object with the
  // same/ name.
  static void unlink(std::string_view name);

private:
  sem_t *semaphore_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_SCREEN_DRAWER_LOCK_POSIX_H_
