/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_TYPE_TRAITS_H_
#define TRAA_BASE_TYPE_TRAITS_H_

#include <cstddef>
#include <string>
#include <type_traits>

namespace traa {
namespace base {

// Determines if the given class has zero-argument .data() and .size() methods
// whose return values are convertible to T* and size_t, respectively.
template <typename DS, typename T> class has_data_and_size {
private:
  template <typename C,
            typename std::enable_if<
                std::is_convertible<decltype(std::declval<C>().data()), T *>::value &&
                std::is_convertible<decltype(std::declval<C>().size()), std::size_t>::value>::type
                * = nullptr>
  static int test(int);

  template <typename> static char test(...);

public:
  static constexpr bool value = std::is_same<decltype(test<DS>(0)), int>::value;
};

namespace test_has_data_and_size {

template <typename DR, typename SR> struct test1 {
  DR data();
  SR size();
};
static_assert(has_data_and_size<test1<int *, int>, int>::value, "");
static_assert(has_data_and_size<test1<int *, int>, const int>::value, "");
static_assert(has_data_and_size<test1<const int *, int>, const int>::value, "");
static_assert(!has_data_and_size<test1<const int *, int>, int>::value,
              "implicit cast of const int* to int*");
static_assert(!has_data_and_size<test1<char *, size_t>, int>::value,
              "implicit cast of char* to int*");

struct test2 {
  int *data;
  size_t size;
};
static_assert(!has_data_and_size<test2, int>::value, ".data and .size aren't functions");

struct test3 {
  int *data();
};
static_assert(!has_data_and_size<test3, int>::value, ".size() is missing");

class test4 {
  int *data();
  size_t size();
};
static_assert(!has_data_and_size<test4, int>::value, ".data() and .size() are private");

} // namespace test_has_data_and_size

namespace type_traits_impl {

// Determines if the given type is an enum that converts implicitly to
// an integral type.
template <typename T> struct is_int_enum {
private:
  // This overload is used if the type is an enum, and unary plus
  // compiles and turns it into an integral type.
  template <typename X,
            typename std::enable_if<std::is_enum<X>::value &&
                                    std::is_integral<decltype(+std::declval<X>())>::value>::type * =
                nullptr>
  static int test(int);

  // Otherwise, this overload is used.
  template <typename> static char test(...);

public:
  static constexpr bool value =
      std::is_same<decltype(test<typename std::remove_reference<T>::type>(0)), int>::value;
};

} // namespace type_traits_impl

// Determines if the given type is integral, or an enum that
// converts implicitly to an integral type.
template <typename T> struct is_int_like {
private:
  using X = typename std::remove_reference<T>::type;

public:
  static constexpr bool value =
      std::is_integral<X>::value || type_traits_impl::is_int_enum<X>::value;
};

namespace test_enum_intlike {

enum enum_1 { e1 };
enum { e2 };
enum class enum_3 { e3 };
struct s {};

static_assert(type_traits_impl::is_int_enum<enum_1>::value, "");
static_assert(type_traits_impl::is_int_enum<decltype(e2)>::value, "");
static_assert(!type_traits_impl::is_int_enum<enum_3>::value, "");
static_assert(!type_traits_impl::is_int_enum<int>::value, "");
static_assert(!type_traits_impl::is_int_enum<float>::value, "");
static_assert(!type_traits_impl::is_int_enum<s>::value, "");

static_assert(is_int_like<enum_1>::value, "");
static_assert(is_int_like<decltype(e2)>::value, "");
static_assert(!is_int_like<enum_3>::value, "");
static_assert(is_int_like<int>::value, "");
static_assert(!is_int_like<float>::value, "");
static_assert(!is_int_like<s>::value, "");

} // namespace test_enum_intlike

} // namespace base
} // namespace traa

#endif // TRAA_BASE_TYPE_TRAITS_H_