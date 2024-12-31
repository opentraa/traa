/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/checks.h"

#include "base/platform.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::testing::HasSubstr;
using ::testing::Not;

TEST(checks_test, expression_not_evaluated_when_check_passing) {
  int i = 0;
  TRAA_CHECK(true) << "i=" << ++i;
  TRAA_CHECK_EQ(i, 0) << "previous check passed, but i was incremented!";
}

#if GTEST_HAS_DEATH_TEST && !defined(TRAA_OS_LINUX_ANDROID)

// TODO @sylar: disable death test for now
TEST(checks_death_test, DISABLED_death_checks) {
#if TRAA_CHECK_MSG_ENABLED
  EXPECT_DEATH(TRAA_FATAL() << "message", "\n\n#\n"
                                          "# fatal error in: \\S+, line \\w+\n"
                                          "# last system error: \\w+\n"
                                          "# check failed: FATAL\\(\\)\n"
                                          "# message");

  int a = 1, b = 2;
  EXPECT_DEATH(TRAA_CHECK_EQ(a, b) << 1 << 2u, "\n\n#\n"
                                               "# fatal error in: \\S+, line \\w+\n"
                                               "# last system error: \\w+\n"
                                               "# check failed: a == b \\(1 vs. 2\\)\n"
                                               "# 12");
  TRAA_CHECK_EQ(5, 5);

  TRAA_CHECK(true) << "Shouldn't crash" << 1;
  EXPECT_DEATH(TRAA_CHECK(false) << "Hi there!", "\n\n#\n"
                                                 "# fatal error in: \\S+, line \\w+\n"
                                                 "# last system error: \\w+\n"
                                                 "# check failed: false\n"
                                                 "# Hi there!");

#else
  EXPECT_DEATH(TRAA_FATAL() << "message", "\n\n#\n"
                                          "# fatal error in: \\S+, line \\w+\n"
                                          "# last system error: \\w+\n"
                                          "# check failed.\n"
                                          "# ");

  int a = 1, b = 2;
  EXPECT_DEATH(TRAA_CHECK_EQ(a, b) << 1 << 2u, "\n\n#\n"
                                               "# fatal error in: \\S+, line \\w+\n"
                                               "# last system error: \\w+\n"
                                               "# check failed.\n"
                                               "# ");
  TRAA_CHECK_EQ(5, 5);

  TRAA_CHECK(true) << "Shouldn't crash" << 1;
  EXPECT_DEATH(TRAA_CHECK(false) << "Hi there!", "\n\n#\n"
                                                 "# fatal error in: \\S+, line \\w+\n"
                                                 "# last system error: \\w+\n"
                                                 "# check failed.\n"
                                                 "# ");
#endif // TRAA_CHECK_MSG_ENABLED
}
#endif // GTEST_HAS_DEATH_TEST && !defined(TRAA_OS_LINUX_ANDROID)

} // namespace
