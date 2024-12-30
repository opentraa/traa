/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file defines six constexpr functions:
//
//   traa::base::safe_eq  // ==
//   traa::base::safe_ne  // !=
//   traa::base::safe_lt  // <
//   traa::base::safe_le  // <=
//   traa::base::safe_gt  // >
//   traa::base::safe_ge  // >=
//
// They each accept two arguments of arbitrary types, and in almost all cases,
// they simply call the appropriate comparison operator. However, if both
// arguments are integers, they don't compare them using C++'s quirky rules,
// but instead adhere to the true mathematical definitions. It is as if the
// arguments were first converted to infinite-range signed integers, and then
// compared, although of course nothing expensive like that actually takes
// place. In practice, for signed/signed and unsigned/unsigned comparisons and
// some mixed-signed comparisons with a compile-time constant, the overhead is
// zero; in the remaining cases, it is just a few machine instructions (no
// branches).

#ifndef TRAA_BASE_NUMERICS_SAFE_COMPARE_H_
#define TRAA_BASE_NUMERICS_SAFE_COMPARE_H_

#include <stddef.h>
#include <stdint.h>

#include <type_traits>

#include "base/type_traits.h"

namespace traa {
namespace base {

namespace safe_cmp_impl {

template <size_t N> struct larger_int_impl : std::false_type {};
template <> struct larger_int_impl<sizeof(int8_t)> : std::true_type {
  using type = int16_t;
};
template <> struct larger_int_impl<sizeof(int16_t)> : std::true_type {
  using type = int32_t;
};
template <> struct larger_int_impl<sizeof(int32_t)> : std::true_type {
  using type = int64_t;
};

// larger_int<T1, T2>::value is true iff there's a signed type that's larger
// than T1 (and no larger than the larger of T2 and int*, for performance
// reasons); and if there is such a type, larger_int<T1, T2>::type is an alias
// for it.
template <typename T1, typename T2>
struct larger_int
    : larger_int_impl<sizeof(T1) < sizeof(T2) || sizeof(T1) < sizeof(int *) ? sizeof(T1) : 0> {};

template <typename T> constexpr typename std::make_unsigned<T>::type make_unsigned(T a) {
  return static_cast<typename std::make_unsigned<T>::type>(a);
}

// Overload for when both T1 and T2 have the same signedness.
template <typename OP, typename T1, typename T2,
          typename std::enable_if<std::is_signed<T1>::value == std::is_signed<T2>::value>::type * =
              nullptr>
constexpr bool cmp(T1 a, T2 b) {
  return OP::safe_op(a, b);
}

// Overload for signed - unsigned comparison that can be promoted to a bigger
// signed type.
template <typename OP, typename T1, typename T2,
          typename std::enable_if<std::is_signed<T1>::value && std::is_unsigned<T2>::value &&
                                  larger_int<T2, T1>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b) {
  return OP::safe_op(a, static_cast<typename larger_int<T2, T1>::type>(b));
}

// Overload for unsigned - signed comparison that can be promoted to a bigger
// signed type.
template <typename OP, typename T1, typename T2,
          typename std::enable_if<std::is_unsigned<T1>::value && std::is_signed<T2>::value &&
                                  larger_int<T1, T2>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b) {
  return OP::safe_op(static_cast<typename larger_int<T1, T2>::type>(a), b);
}

// Overload for signed - unsigned comparison that can't be promoted to a bigger
// signed type.
template <typename OP, typename T1, typename T2,
          typename std::enable_if<std::is_signed<T1>::value && std::is_unsigned<T2>::value &&
                                  !larger_int<T2, T1>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b) {
  return a < 0 ? OP::safe_op(-1, 0) : OP::safe_op(safe_cmp_impl::make_unsigned(a), b);
}

// Overload for unsigned - signed comparison that can't be promoted to a bigger
// signed type.
template <typename OP, typename T1, typename T2,
          typename std::enable_if<std::is_unsigned<T1>::value && std::is_signed<T2>::value &&
                                  !larger_int<T1, T2>::value>::type * = nullptr>
constexpr bool cmp(T1 a, T2 b) {
  return b < 0 ? OP::safe_op(0, -1) : OP::safe_op(a, safe_cmp_impl::make_unsigned(b));
}

#define TRAA_SAFECMP_MAKE_OP(name, op)                                                             \
  struct name {                                                                                    \
    template <typename T1, typename T2> static constexpr bool safe_op(T1 a, T2 b) {                \
      return a op b;                                                                               \
    }                                                                                              \
  };
TRAA_SAFECMP_MAKE_OP(eq_op, ==)
TRAA_SAFECMP_MAKE_OP(ne_op, !=)
TRAA_SAFECMP_MAKE_OP(lt_op, <)
TRAA_SAFECMP_MAKE_OP(le_op, <=)
TRAA_SAFECMP_MAKE_OP(gt_op, >)
TRAA_SAFECMP_MAKE_OP(ge_op, >=)
#undef TRAA_SAFECMP_MAKE_OP

} // namespace safe_cmp_impl

#define TRAA_SAFECMP_MAKE_FUN(name)                                                                \
  template <typename T1, typename T2>                                                              \
  constexpr typename std::enable_if<is_int_like<T1>::value && is_int_like<T2>::value, bool>::type  \
      safe_##name(T1 a, T2 b) {                                                                    \
    /* Unary plus here turns enums into real integral types. */                                    \
    return safe_cmp_impl::cmp<safe_cmp_impl::name##_op>(+a, +b);                                   \
  }                                                                                                \
  template <typename T1, typename T2>                                                              \
  constexpr                                                                                        \
      typename std::enable_if<!is_int_like<T1>::value || !is_int_like<T2>::value, bool>::type      \
          safe_##name(const T1 &a, const T2 &b) {                                                  \
    return safe_cmp_impl::name##_op::safe_op(a, b);                                                \
  }
TRAA_SAFECMP_MAKE_FUN(eq)
TRAA_SAFECMP_MAKE_FUN(ne)
TRAA_SAFECMP_MAKE_FUN(lt)
TRAA_SAFECMP_MAKE_FUN(le)
TRAA_SAFECMP_MAKE_FUN(gt)
TRAA_SAFECMP_MAKE_FUN(ge)
#undef TRAA_SAFECMP_MAKE_FUN

} // namespace base
} // namespace traa

#endif // TRAA_BASE_NUMERICS_SAFE_COMPARE_H_