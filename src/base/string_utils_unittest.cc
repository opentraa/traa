/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/string_utils.h"

#include <gtest/gtest.h>

namespace traa {
namespace base {

TEST(string_to_hex_test, to_hex) {
  EXPECT_EQ(to_hex(0), "0");
  EXPECT_EQ(to_hex(0X1243E), "1243e");
  EXPECT_EQ(to_hex(-20), "ffffffec");
}

#if defined(TRAA_OS_WINDOWS)

TEST(string_to_utf_test, empty) {
  char empty_string[] = "";
  EXPECT_TRUE(to_utf16(empty_string, 0u).empty());
  wchar_t empty_wchar[] = L"";
  EXPECT_TRUE(to_utf8(empty_wchar, 0u).empty());
}

#endif // TRAA_OS_WINDOWS

TEST(compile_time_string_test, make_acts_like_a_string) {
  EXPECT_STREQ(make_compile_time_string("abc123"), "abc123");
}

TEST(compile_time_string_test, convertible_to_std_string) {
  EXPECT_EQ(std::string(make_compile_time_string("abab")), "abab");
}

namespace detail {
constexpr bool string_equals(const char *a, const char *b) {
  while (*a && *a == *b)
    a++, b++;
  return *a == *b;
}
} // namespace detail

static_assert(detail::string_equals(make_compile_time_string("handellm"), "handellm"),
              "String should initialize.");

static_assert(detail::string_equals(
                  make_compile_time_string("abc123").concat(make_compile_time_string("def456ghi")),
                  "abc123def456ghi"),
              "Strings should concatenate.");

} // namespace base
} // namespace traa
