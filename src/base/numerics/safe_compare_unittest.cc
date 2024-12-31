/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/numerics/safe_compare.h"

#include <cstdint>
#include <limits>
#include <utility>

#include <gtest/gtest.h>

namespace traa {
namespace base {

namespace {

constexpr std::uintmax_t umax = std::numeric_limits<std::uintmax_t>::max();
constexpr std::intmax_t imin = std::numeric_limits<std::intmax_t>::min();
constexpr std::intmax_t m1 = -1;

// m1 and umax have the same representation because we use 2's complement
// arithmetic, so naive casting will confuse them.
static_assert(static_cast<std::uintmax_t>(m1) == umax, "");
static_assert(m1 == static_cast<std::intmax_t>(umax), "");

static const std::pair<int, int> p1(1, 1);
static const std::pair<int, int> p2(1, 2);

} // namespace

// clang-format off

// These functions aren't used in the tests, but it's useful to look at the
// compiler output for them, and verify that (1) the same-signedness *safe
// functions result in exactly the same code as their *ref counterparts, and
// that (2) the mixed-signedness *safe functions have just a few extra
// arithmetic and logic instructions (but no extra control flow instructions).
bool test_less_than_ref(      int a,      int b) { return a < b; }
bool test_less_than_ref( unsigned a, unsigned b) { return a < b; }
bool test_less_than_safe(     int a,      int b) { return safe_lt(a, b); }
bool test_less_than_safe(unsigned a, unsigned b) { return safe_lt(a, b); }
bool test_less_than_safe(unsigned a,      int b) { return safe_lt(a, b); }
bool test_less_than_safe(     int a, unsigned b) { return safe_lt(a, b); }

// For these, we expect the *ref and *safe functions to result in identical
// code, except for the ones that compare a signed variable with an unsigned
// constant; in that case, the *ref function does an unsigned comparison (fast
// but incorrect) and the *safe function spends a few extra instructions on
// doing it right.
bool test_less_than_17_ref(       int a) { return a < 17; }
bool test_less_than_17_ref(  unsigned a) { return a < 17; }
bool test_less_than_17u_ref(      int a) { return static_cast<unsigned>(a) < 17u; }
bool test_less_than_17u_ref( unsigned a) { return a < 17u; }
bool test_less_than_17_safe(      int a) { return safe_lt(a, 17); }
bool test_less_than_17_safe( unsigned a) { return safe_lt(a, 17); }
bool test_less_than_17u_safe(     int a) { return safe_lt(a, 17u); }
bool test_less_than_17u_safe(unsigned a) { return safe_lt(a, 17u); }

// Cases where we can't convert to a larger signed type.
bool test_less_than_max( intmax_t a, uintmax_t b) { return safe_lt(a, b); }
bool test_less_than_max(uintmax_t a,  intmax_t b) { return safe_lt(a, b); }
bool test_less_than_max_17u( intmax_t a) { return safe_lt(a, uintmax_t{17}); }
bool test_less_than_max_17( uintmax_t a) { return safe_lt(a,  intmax_t{17}); }

// Cases where the compiler should be able to compute the result at compile
// time.
bool test_less_than_const_1() { return safe_lt(  -1,    1); }
bool test_less_than_const_2() { return safe_lt(  m1, umax); }
bool test_less_than_const_3() { return safe_lt(umax, imin); }
bool test_less_than_const_4(unsigned a) { return safe_lt( a, -1); }
bool test_less_than_const_5(unsigned a) { return safe_lt(-1,  a); }
bool test_less_than_const_6(unsigned a) { return safe_lt( a,  a); }

// clang-format on

TEST(safe_cmp_test, eq) {
  static_assert(!safe_eq(-1, 2), "");
  static_assert(!safe_eq(-1, 2u), "");
  static_assert(!safe_eq(2, -1), "");
  static_assert(!safe_eq(2u, -1), "");

  static_assert(!safe_eq(1, 2), "");
  static_assert(!safe_eq(1, 2u), "");
  static_assert(!safe_eq(1u, 2), "");
  static_assert(!safe_eq(1u, 2u), "");
  static_assert(!safe_eq(2, 1), "");
  static_assert(!safe_eq(2, 1u), "");
  static_assert(!safe_eq(2u, 1), "");
  static_assert(!safe_eq(2u, 1u), "");

  static_assert(safe_eq(2, 2), "");
  static_assert(safe_eq(2, 2u), "");
  static_assert(safe_eq(2u, 2), "");
  static_assert(safe_eq(2u, 2u), "");

  static_assert(safe_eq(imin, imin), "");
  static_assert(!safe_eq(imin, umax), "");
  static_assert(!safe_eq(umax, imin), "");
  static_assert(safe_eq(umax, umax), "");

  static_assert(safe_eq(m1, m1), "");
  static_assert(!safe_eq(m1, umax), "");
  static_assert(!safe_eq(umax, m1), "");
  static_assert(safe_eq(umax, umax), "");

  static_assert(!safe_eq(1, 2), "");
  static_assert(!safe_eq(1, 2.0), "");
  static_assert(!safe_eq(1.0, 2), "");
  static_assert(!safe_eq(1.0, 2.0), "");
  static_assert(!safe_eq(2, 1), "");
  static_assert(!safe_eq(2, 1.0), "");
  static_assert(!safe_eq(2.0, 1), "");
  static_assert(!safe_eq(2.0, 1.0), "");

  static_assert(safe_eq(2, 2), "");
  static_assert(safe_eq(2, 2.0), "");
  static_assert(safe_eq(2.0, 2), "");
  static_assert(safe_eq(2.0, 2.0), "");

  EXPECT_TRUE(safe_eq(p1, p1));
  EXPECT_FALSE(safe_eq(p1, p2));
  EXPECT_FALSE(safe_eq(p2, p1));
  EXPECT_TRUE(safe_eq(p2, p2));
}

TEST(safe_cmp_test, ne) {
  static_assert(safe_ne(-1, 2), "");
  static_assert(safe_ne(-1, 2u), "");
  static_assert(safe_ne(2, -1), "");
  static_assert(safe_ne(2u, -1), "");

  static_assert(safe_ne(1, 2), "");
  static_assert(safe_ne(1, 2u), "");
  static_assert(safe_ne(1u, 2), "");
  static_assert(safe_ne(1u, 2u), "");
  static_assert(safe_ne(2, 1), "");
  static_assert(safe_ne(2, 1u), "");
  static_assert(safe_ne(2u, 1), "");
  static_assert(safe_ne(2u, 1u), "");

  static_assert(!safe_ne(2, 2), "");
  static_assert(!safe_ne(2, 2u), "");
  static_assert(!safe_ne(2u, 2), "");
  static_assert(!safe_ne(2u, 2u), "");

  static_assert(!safe_ne(imin, imin), "");
  static_assert(safe_ne(imin, umax), "");
  static_assert(safe_ne(umax, imin), "");
  static_assert(!safe_ne(umax, umax), "");

  static_assert(!safe_ne(m1, m1), "");
  static_assert(safe_ne(m1, umax), "");
  static_assert(safe_ne(umax, m1), "");
  static_assert(!safe_ne(umax, umax), "");

  static_assert(safe_ne(1, 2), "");
  static_assert(safe_ne(1, 2.0), "");
  static_assert(safe_ne(1.0, 2), "");
  static_assert(safe_ne(1.0, 2.0), "");
  static_assert(safe_ne(2, 1), "");
  static_assert(safe_ne(2, 1.0), "");
  static_assert(safe_ne(2.0, 1), "");
  static_assert(safe_ne(2.0, 1.0), "");

  static_assert(!safe_ne(2, 2), "");
  static_assert(!safe_ne(2, 2.0), "");
  static_assert(!safe_ne(2.0, 2), "");
  static_assert(!safe_ne(2.0, 2.0), "");

  EXPECT_FALSE(safe_ne(p1, p1));
  EXPECT_TRUE(safe_ne(p1, p2));
  EXPECT_TRUE(safe_ne(p2, p1));
  EXPECT_FALSE(safe_ne(p2, p2));
}

TEST(safe_cmp_test, lt) {
  static_assert(safe_lt(-1, 2), "");
  static_assert(safe_lt(-1, 2u), "");
  static_assert(!safe_lt(2, -1), "");
  static_assert(!safe_lt(2u, -1), "");

  static_assert(safe_lt(1, 2), "");
  static_assert(safe_lt(1, 2u), "");
  static_assert(safe_lt(1u, 2), "");
  static_assert(safe_lt(1u, 2u), "");
  static_assert(!safe_lt(2, 1), "");
  static_assert(!safe_lt(2, 1u), "");
  static_assert(!safe_lt(2u, 1), "");
  static_assert(!safe_lt(2u, 1u), "");

  static_assert(!safe_lt(2, 2), "");
  static_assert(!safe_lt(2, 2u), "");
  static_assert(!safe_lt(2u, 2), "");
  static_assert(!safe_lt(2u, 2u), "");

  static_assert(!safe_lt(imin, imin), "");
  static_assert(safe_lt(imin, umax), "");
  static_assert(!safe_lt(umax, imin), "");
  static_assert(!safe_lt(umax, umax), "");

  static_assert(!safe_lt(m1, m1), "");
  static_assert(safe_lt(m1, umax), "");
  static_assert(!safe_lt(umax, m1), "");
  static_assert(!safe_lt(umax, umax), "");

  static_assert(safe_lt(1, 2), "");
  static_assert(safe_lt(1, 2.0), "");
  static_assert(safe_lt(1.0, 2), "");
  static_assert(safe_lt(1.0, 2.0), "");
  static_assert(!safe_lt(2, 1), "");
  static_assert(!safe_lt(2, 1.0), "");
  static_assert(!safe_lt(2.0, 1), "");
  static_assert(!safe_lt(2.0, 1.0), "");

  static_assert(!safe_lt(2, 2), "");
  static_assert(!safe_lt(2, 2.0), "");
  static_assert(!safe_lt(2.0, 2), "");
  static_assert(!safe_lt(2.0, 2.0), "");

  EXPECT_FALSE(safe_lt(p1, p1));
  EXPECT_TRUE(safe_lt(p1, p2));
  EXPECT_FALSE(safe_lt(p2, p1));
  EXPECT_FALSE(safe_lt(p2, p2));
}

TEST(safe_cmp_test, le) {
  static_assert(safe_le(-1, 2), "");
  static_assert(safe_le(-1, 2u), "");
  static_assert(!safe_le(2, -1), "");
  static_assert(!safe_le(2u, -1), "");

  static_assert(safe_le(1, 2), "");
  static_assert(safe_le(1, 2u), "");
  static_assert(safe_le(1u, 2), "");
  static_assert(safe_le(1u, 2u), "");
  static_assert(!safe_le(2, 1), "");
  static_assert(!safe_le(2, 1u), "");
  static_assert(!safe_le(2u, 1), "");
  static_assert(!safe_le(2u, 1u), "");

  static_assert(safe_le(2, 2), "");
  static_assert(safe_le(2, 2u), "");
  static_assert(safe_le(2u, 2), "");
  static_assert(safe_le(2u, 2u), "");

  static_assert(safe_le(imin, imin), "");
  static_assert(safe_le(imin, umax), "");
  static_assert(!safe_le(umax, imin), "");
  static_assert(safe_le(umax, umax), "");

  static_assert(safe_le(m1, m1), "");
  static_assert(safe_le(m1, umax), "");
  static_assert(!safe_le(umax, m1), "");
  static_assert(safe_le(umax, umax), "");

  static_assert(safe_le(1, 2), "");
  static_assert(safe_le(1, 2.0), "");
  static_assert(safe_le(1.0, 2), "");
  static_assert(safe_le(1.0, 2.0), "");
  static_assert(!safe_le(2, 1), "");
  static_assert(!safe_le(2, 1.0), "");
  static_assert(!safe_le(2.0, 1), "");
  static_assert(!safe_le(2.0, 1.0), "");

  static_assert(safe_le(2, 2), "");
  static_assert(safe_le(2, 2.0), "");
  static_assert(safe_le(2.0, 2), "");
  static_assert(safe_le(2.0, 2.0), "");

  EXPECT_TRUE(safe_le(p1, p1));
  EXPECT_TRUE(safe_le(p1, p2));
  EXPECT_FALSE(safe_le(p2, p1));
  EXPECT_TRUE(safe_le(p2, p2));
}

TEST(safe_cmp_test, gt) {
  static_assert(!safe_gt(-1, 2), "");
  static_assert(!safe_gt(-1, 2u), "");
  static_assert(safe_gt(2, -1), "");
  static_assert(safe_gt(2u, -1), "");

  static_assert(!safe_gt(1, 2), "");
  static_assert(!safe_gt(1, 2u), "");
  static_assert(!safe_gt(1u, 2), "");
  static_assert(!safe_gt(1u, 2u), "");
  static_assert(safe_gt(2, 1), "");
  static_assert(safe_gt(2, 1u), "");
  static_assert(safe_gt(2u, 1), "");
  static_assert(safe_gt(2u, 1u), "");

  static_assert(!safe_gt(2, 2), "");
  static_assert(!safe_gt(2, 2u), "");
  static_assert(!safe_gt(2u, 2), "");
  static_assert(!safe_gt(2u, 2u), "");

  static_assert(!safe_gt(imin, imin), "");
  static_assert(!safe_gt(imin, umax), "");
  static_assert(safe_gt(umax, imin), "");
  static_assert(!safe_gt(umax, umax), "");

  static_assert(!safe_gt(m1, m1), "");
  static_assert(!safe_gt(m1, umax), "");
  static_assert(safe_gt(umax, m1), "");
  static_assert(!safe_gt(umax, umax), "");

  static_assert(!safe_gt(1, 2), "");
  static_assert(!safe_gt(1, 2.0), "");
  static_assert(!safe_gt(1.0, 2), "");
  static_assert(!safe_gt(1.0, 2.0), "");
  static_assert(safe_gt(2, 1), "");
  static_assert(safe_gt(2, 1.0), "");
  static_assert(safe_gt(2.0, 1), "");
  static_assert(safe_gt(2.0, 1.0), "");

  static_assert(!safe_gt(2, 2), "");
  static_assert(!safe_gt(2, 2.0), "");
  static_assert(!safe_gt(2.0, 2), "");
  static_assert(!safe_gt(2.0, 2.0), "");

  EXPECT_FALSE(safe_gt(p1, p1));
  EXPECT_FALSE(safe_gt(p1, p2));
  EXPECT_TRUE(safe_gt(p2, p1));
  EXPECT_FALSE(safe_gt(p2, p2));
}

TEST(safe_cmp_test, ge) {
  static_assert(!safe_ge(-1, 2), "");
  static_assert(!safe_ge(-1, 2u), "");
  static_assert(safe_ge(2, -1), "");
  static_assert(safe_ge(2u, -1), "");

  static_assert(!safe_ge(1, 2), "");
  static_assert(!safe_ge(1, 2u), "");
  static_assert(!safe_ge(1u, 2), "");
  static_assert(!safe_ge(1u, 2u), "");
  static_assert(safe_ge(2, 1), "");
  static_assert(safe_ge(2, 1u), "");
  static_assert(safe_ge(2u, 1), "");
  static_assert(safe_ge(2u, 1u), "");

  static_assert(safe_ge(2, 2), "");
  static_assert(safe_ge(2, 2u), "");
  static_assert(safe_ge(2u, 2), "");
  static_assert(safe_ge(2u, 2u), "");

  static_assert(safe_ge(imin, imin), "");
  static_assert(!safe_ge(imin, umax), "");
  static_assert(safe_ge(umax, imin), "");
  static_assert(safe_ge(umax, umax), "");

  static_assert(safe_ge(m1, m1), "");
  static_assert(!safe_ge(m1, umax), "");
  static_assert(safe_ge(umax, m1), "");
  static_assert(safe_ge(umax, umax), "");

  static_assert(!safe_ge(1, 2), "");
  static_assert(!safe_ge(1, 2.0), "");
  static_assert(!safe_ge(1.0, 2), "");
  static_assert(!safe_ge(1.0, 2.0), "");
  static_assert(safe_ge(2, 1), "");
  static_assert(safe_ge(2, 1.0), "");
  static_assert(safe_ge(2.0, 1), "");
  static_assert(safe_ge(2.0, 1.0), "");

  static_assert(safe_ge(2, 2), "");
  static_assert(safe_ge(2, 2.0), "");
  static_assert(safe_ge(2.0, 2), "");
  static_assert(safe_ge(2.0, 2.0), "");

  EXPECT_TRUE(safe_ge(p1, p1));
  EXPECT_FALSE(safe_ge(p1, p2));
  EXPECT_TRUE(safe_ge(p2, p1));
  EXPECT_TRUE(safe_ge(p2, p2));
}

TEST(safe_cmp_test, enum) {
  enum E1 { e1 = 13 };
  enum { e2 = 13 };
  enum E3 : unsigned { e3 = 13 };
  enum : unsigned { e4 = 13 };
  static_assert(safe_eq(13, e1), "");
  static_assert(safe_eq(13u, e1), "");
  static_assert(safe_eq(13, e2), "");
  static_assert(safe_eq(13u, e2), "");
  static_assert(safe_eq(13, e3), "");
  static_assert(safe_eq(13u, e3), "");
  static_assert(safe_eq(13, e4), "");
  static_assert(safe_eq(13u, e4), "");
}

} // namespace base
} // namespace traa
