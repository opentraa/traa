/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_FUNCTION_VIEW_H_
#define TRAA_BASE_FUNCTION_VIEW_H_

#include <cstddef>
#include <type_traits>
#include <utility>

// Just like std::function, function_view will wrap any callable and hide its
// actual type, exposing only its signature. But unlike std::function,
// function_view doesn't own its callable---it just points to it. Thus, it's a
// good choice mainly as a function argument when the callable argument will
// not be called again once the function has returned.
//
// Its constructors are implicit, so that callers won't have to convert lambdas
// and other callables to function_view<Blah(Blah, Blah)> explicitly. This is
// safe because function_view is only a reference to the real callable.
//
// Example use:
//
//   void SomeFunction(traa::base::function_view<int(int)> index_transform);
//   ...
//   SomeFunction([](int i) { return 2 * i + 1; });
//
// Note: function_view is tiny (essentially just two pointers) and trivially
// copyable, so it's probably cheaper to pass it by value than by const
// reference.

namespace traa {
namespace base {

template <typename T> class function_view; // Undefined.

template <typename RetT, typename... ArgT> class function_view<RetT(ArgT...)> final {
public:
  // Constructor for lambdas and other callables; it accepts every type of
  // argument except those noted in its enable_if call.
  template <typename F,
            typename std::enable_if<
                // Not for function pointers; we have another constructor for that
                // below.
                !std::is_function<typename std::remove_pointer<
                    typename std::remove_reference<F>::type>::type>::value &&

                // Not for nullptr; we have another constructor for that below.
                !std::is_same<std::nullptr_t, typename std::remove_cv<F>::type>::value &&

                // Not for function_view objects; we have another constructor for that
                // (the implicitly declared copy constructor).
                !std::is_same<function_view, typename std::remove_cv<typename std::remove_reference<
                                                 F>::type>::type>::value>::type * = nullptr>
  function_view(F &&f) : call_(call_void_ptr<typename std::remove_reference<F>::type>) {
    f_.void_ptr = &f;
  }

  // Constructor that accepts function pointers. If the argument is null, the
  // result is an empty function_view.
  template <typename F,
            typename std::enable_if<std::is_function<typename std::remove_pointer<
                typename std::remove_reference<F>::type>::type>::value>::type * = nullptr>
  function_view(F &&f) : call_(f ? call_func_ptr<typename std::remove_pointer<F>::type> : nullptr) {
    f_.fun_ptr = reinterpret_cast<void (*)()>(f);
  }

  // Constructor that accepts nullptr. It creates an empty function_view.
  template <typename F,
            typename std::enable_if<std::is_same<
                std::nullptr_t, typename std::remove_cv<F>::type>::value>::type * = nullptr>
  function_view(F && /* f */) : call_(nullptr) {}

  // Default constructor. Creates an empty function_view.
  function_view() : call_(nullptr) {}

  RetT operator()(ArgT... args) const { return call_(f_, std::forward<ArgT>(args)...); }

  // Returns true if we have a function, false if we don't (i.e., we're null).
  explicit operator bool() const { return !!call_; }

private:
  union void_union {
    void *void_ptr;
    void (*fun_ptr)();
  };

  template <typename F> static RetT call_void_ptr(void_union vu, ArgT... args) {
    return (*static_cast<F *>(vu.void_ptr))(std::forward<ArgT>(args)...);
  }
  template <typename F> static RetT call_func_ptr(void_union vu, ArgT... args) {
    return (reinterpret_cast<typename std::add_pointer<F>::type>(vu.fun_ptr))(
        std::forward<ArgT>(args)...);
  }

  // A pointer to the callable thing, with type information erased. It's a
  // union because we have to use separate types depending on if the callable
  // thing is a function pointer or something else.
  void_union f_;

  // Pointer to a dispatch function that knows the type of the callable thing
  // that's stored in f_, and how to call it. A function_view object is empty
  // (null) iff call_ is null.
  RetT (*call_)(void_union, ArgT...);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_FUNCTION_VIEW_H_
