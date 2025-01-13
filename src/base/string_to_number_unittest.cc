/*
 *  Copyright 2017 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/string_to_number.h"

#include <stdint.h>

#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace traa {
namespace base {

namespace {
// clang-format off
using integer_types =
    ::testing::Types<char,
                     signed char, unsigned char,       // NOLINT(runtime/int)
                     short,       unsigned short,      // NOLINT(runtime/int)
                     int,         unsigned int,        // NOLINT(runtime/int)
                     long,        unsigned long,       // NOLINT(runtime/int)
                     long long,   unsigned long long,  // NOLINT(runtime/int)
                     int8_t,      uint8_t,
                     int16_t,     uint16_t,
                     int32_t,     uint32_t,
                     int64_t,     uint64_t>;
// clang-format on

template <typename T> class basic_number_test : public ::testing::Test {};

TYPED_TEST_SUITE_P(basic_number_test);

TYPED_TEST_P(basic_number_test, test_valid_numbers) {
  using T = TypeParam;
  constexpr T min_value = std::numeric_limits<T>::lowest();
  constexpr T max_value = std::numeric_limits<T>::max();
  constexpr T zero_value = 0;
  const std::string min_string = std::to_string(min_value);
  const std::string max_string = std::to_string(max_value);
  EXPECT_EQ(min_value, string_to_number<T>(min_string));
  EXPECT_EQ(min_value, string_to_number<T>(min_string.c_str()));
  EXPECT_EQ(max_value, string_to_number<T>(max_string));
  EXPECT_EQ(max_value, string_to_number<T>(max_string.c_str()));
  EXPECT_EQ(zero_value, string_to_number<T>("0"));
  EXPECT_EQ(zero_value, string_to_number<T>("-0"));
  EXPECT_EQ(zero_value, string_to_number<T>(std::string("-0000000000000")));
}

TYPED_TEST_P(basic_number_test, test_invalid_numbers) {
  using T = TypeParam;
  // Value ranges aren't strictly enforced in this test, since that would either
  // require doctoring specific strings for each data type, which is a hassle
  // across platforms, or to be able to do addition of values larger than the
  // largest type, which is another hassle.
  constexpr T min_value = std::numeric_limits<T>::lowest();
  constexpr T max_value = std::numeric_limits<T>::max();
  // If the type supports negative values, make the large negative value
  // approximately ten times larger. If the type is unsigned, just use -2.
  const std::string too_low_string = (min_value == 0) ? "-2" : (std::to_string(min_value) + "1");
  // Make the large value approximately ten times larger than the maximum.
  const std::string too_large_string = std::to_string(max_value) + "1";
  EXPECT_EQ(std::nullopt, string_to_number<T>(too_low_string));
  EXPECT_EQ(std::nullopt, string_to_number<T>(too_low_string.c_str()));
  EXPECT_EQ(std::nullopt, string_to_number<T>(too_large_string));
  EXPECT_EQ(std::nullopt, string_to_number<T>(too_large_string.c_str()));
}

TYPED_TEST_P(basic_number_test, test_invalid_inputs) {
  using T = TypeParam;
  const char k_invalid_char_array[] = "Invalid string containing 47";
  const char k_plus_minus_char_array[] = "+-100";
  const char k_number_followed_by_cruft[] = "640x480";
  const char k_embedded_nul[] = {'1', '2', '\0', '3', '4'};
  const char k_beginning_embedded_nul[] = {'\0', '1', '2', '3', '4'};
  const char k_trailing_embedded_nul[] = {'1', '2', '3', '4', '\0'};

  EXPECT_EQ(std::nullopt, string_to_number<T>(k_invalid_char_array));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string(k_invalid_char_array)));
  EXPECT_EQ(std::nullopt, string_to_number<T>(k_plus_minus_char_array));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string(k_plus_minus_char_array)));
  EXPECT_EQ(std::nullopt, string_to_number<T>(k_number_followed_by_cruft));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string(k_number_followed_by_cruft)));
  EXPECT_EQ(std::nullopt, string_to_number<T>(" 5"));
  EXPECT_EQ(std::nullopt, string_to_number<T>(" - 5"));
  EXPECT_EQ(std::nullopt, string_to_number<T>("- 5"));
  EXPECT_EQ(std::nullopt, string_to_number<T>(" -5"));
  EXPECT_EQ(std::nullopt, string_to_number<T>("5 "));
  // Test various types of empty inputs
  EXPECT_EQ(std::nullopt, string_to_number<T>({nullptr, 0}));
  EXPECT_EQ(std::nullopt, string_to_number<T>(""));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string()));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string("")));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string_view()));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string_view(nullptr, 0)));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string_view("")));
  // Test strings with embedded nuls.
  EXPECT_EQ(std::nullopt,
            string_to_number<T>(std::string_view(k_embedded_nul, sizeof(k_embedded_nul))));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string_view(k_beginning_embedded_nul,
                                                               sizeof(k_beginning_embedded_nul))));
  EXPECT_EQ(std::nullopt, string_to_number<T>(std::string_view(k_trailing_embedded_nul,
                                                               sizeof(k_trailing_embedded_nul))));
}

REGISTER_TYPED_TEST_SUITE_P(basic_number_test, test_valid_numbers, test_invalid_numbers,
                            test_invalid_inputs);

} // namespace

INSTANTIATE_TYPED_TEST_SUITE_P(string_to_number_test_integers, basic_number_test, integer_types);

TEST(string_to_number_test, test_specific_values) {
  EXPECT_EQ(std::nullopt, string_to_number<uint8_t>("256"));
  EXPECT_EQ(std::nullopt, string_to_number<uint8_t>("-256"));
  EXPECT_EQ(std::nullopt, string_to_number<int8_t>("256"));
  EXPECT_EQ(std::nullopt, string_to_number<int8_t>("-256"));
}

} // namespace base
} // namespace traa
