/*
 *  Copyright 2006 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_CHECKS_H_
#define TRAA_BASE_CHECKS_H_

// If you for some reson need to know if DCHECKs are on, test the value of
// TRAA_DCHECK_IS_ON. (Test its value, not if it's defined; it'll always be
// defined, to either a true or a false value.)
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
#define TRAA_DCHECK_IS_ON 1
#else
#define TRAA_DCHECK_IS_ON 0
#endif

// Annotate a function that will not return control flow to the caller.
#if defined(_MSC_VER)
#define TRAA_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define TRAA_NORETURN __attribute__((__noreturn__))
#else
#define TRAA_NORETURN
#endif

#ifdef __cplusplus
extern "C" {
#endif
TRAA_NORETURN void fatal_message(const char *file, int line, const char *msg);
#ifdef __cplusplus
} // extern "C"
#endif

#ifdef TRAA_DISABLE_CHECK_MSG
#define TRAA_CHECK_MSG_ENABLED 0
#else
#define TRAA_CHECK_MSG_ENABLED 1
#endif

#if TRAA_CHECK_MSG_ENABLED
#define TRAA_CHECK_EVAL_MESSAGE(message) message
#else
#define TRAA_CHECK_EVAL_MESSAGE(message) ""
#endif

#ifdef __cplusplus
// C++ version.

#include <memory>
#include <string>
#include <string_view>

#include "base/hedley.h"
#include "base/numerics/safe_compare.h"
#include "base/type_traits.h"

// The macros here print a message to stderr and abort under various
// conditions. All will accept additional stream messages. For example:
// TRAA_DCHECK_EQ(foo, bar) << "I'm printed when foo != bar.";
//
// - TRAA_CHECK(x) is an assertion that x is always true, and that if it isn't,
//   it's better to terminate the process than to continue. During development,
//   the reason that it's better to terminate might simply be that the error
//   handling code isn't in place yet; in production, the reason might be that
//   the author of the code truly believes that x will always be true, but that
//   they recognizes that if they are wrong, abrupt and unpleasant process
//   termination is still better than carrying on with the assumption violated.
//
//   TRAA_CHECK always evaluates its argument, so it's OK for x to have side
//   effects.
//
// - TRAA_DCHECK(x) is the same as TRAA_CHECK(x)---an assertion that x is always
//   true---except that x will only be evaluated in debug builds; in production
//   builds, x is simply assumed to be true. This is useful if evaluating x is
//   expensive and the expected cost of failing to detect the violated
//   assumption is acceptable. You should not handle cases where a production
//   build fails to spot a violated condition, even those that would result in
//   crashes. If the code needs to cope with the error, make it cope, but don't
//   call TRAA_DCHECK; if the condition really can't occur, but you'd sleep
//   better at night knowing that the process will suicide instead of carrying
//   on in case you were wrong, use TRAA_CHECK instead of TRAA_DCHECK.
//
//   TRAA_DCHECK only evaluates its argument in debug builds, so if x has visible
//   side effects, you need to write e.g.
//     bool w = x; TRAA_DCHECK(w);
//
// - TRAA_CHECK_EQ, _NE, _GT, ..., and TRAA_DCHECK_EQ, _NE, _GT, ... are
//   specialized variants of TRAA_CHECK and TRAA_DCHECK that print prettier
//   messages if the condition doesn't hold. Prefer them to raw TRAA_CHECK and
//   TRAA_DCHECK.
//
// - TRAA_FATAL() aborts unconditionally.

namespace traa {
namespace base {

namespace checks_impl {
enum class check_arg_type : int8_t {
  t_end = 0,
  t_int,
  t_long,
  t_long_long,
  t_uint,
  t_ulong,
  t_ulong_long,
  t_double,
  t_long_double,
  t_char_p,
  t_std_string,
  t_string_view,
  t_void_p,

  // t_check_op doesn't represent an argument type. Instead, it is sent as the
  // first argument from TRAA_CHECK_OP to make fatal_log use the next two
  // arguments to build the special CHECK_OP error message
  // (the "a == b (1 vs. 2)" bit).
  t_check_op,
};

#if TRAA_CHECK_MSG_ENABLED
TRAA_NORETURN void fatal_log(const char *file, int line, const char *message,
                             const check_arg_type *fmt, ...);
#else
TRAA_NORETURN void fatal_log(const char *file, int line);
#endif

// Wrapper for log arguments. Only ever make values of this type with the
// make_val() functions.
template <check_arg_type N, typename T> struct val {
  static constexpr check_arg_type Type() { return N; }
  T get_val() const { return val_; }
  T val_;
};

inline val<check_arg_type::t_int, int> make_val(int x) { return {x}; }
inline val<check_arg_type::t_long, long> make_val(long x) { return {x}; }
inline val<check_arg_type::t_long_long, long long> make_val(long long x) { return {x}; }
inline val<check_arg_type::t_uint, unsigned int> make_val(unsigned int x) { return {x}; }
inline val<check_arg_type::t_ulong, unsigned long> make_val(unsigned long x) { return {x}; }
inline val<check_arg_type::t_ulong_long, unsigned long long> make_val(unsigned long long x) {
  return {x};
}

inline val<check_arg_type::t_double, double> make_val(double x) { return {x}; }
inline val<check_arg_type::t_long_double, long double> make_val(long double x) { return {x}; }

inline val<check_arg_type::t_char_p, const char *> make_val(const char *x) { return {x}; }
inline val<check_arg_type::t_std_string, const std::string *> make_val(const std::string &x) {
  return {&x};
}
inline val<check_arg_type::t_string_view, const std::string_view *>
make_val(const std::string_view &x) {
  return {&x};
}

inline val<check_arg_type::t_void_p, const void *> make_val(const void *x) { return {x}; }

template <typename T>
inline val<check_arg_type::t_void_p, const void *> make_val(const std::shared_ptr<T> &p) {
  return {p.get()};
}

// The enum class types are not implicitly convertible to arithmetic types.
template <typename T,
          std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value> * = nullptr>
inline decltype(make_val(std::declval<std::underlying_type_t<T>>())) make_val(T x) {
  return {static_cast<std::underlying_type_t<T>>(x)};
}

// Ephemeral type that represents the result of the logging << operator.
template <typename... Ts> class log_streamer;

// Base case: Before the first << argument.
template <> class log_streamer<> final {
public:
  template <typename U, typename V = decltype(make_val(std::declval<U>())),
            std::enable_if_t<std::is_arithmetic<U>::value || std::is_enum<U>::value> * = nullptr>
  HEDLEY_ALWAYS_INLINE log_streamer<V> operator<<(U arg) const {
    return log_streamer<V>(make_val(arg), this);
  }

  template <typename U, typename V = decltype(make_val(std::declval<U>())),
            std::enable_if_t<!std::is_arithmetic<U>::value && !std::is_enum<U>::value> * = nullptr>
  HEDLEY_ALWAYS_INLINE log_streamer<V> operator<<(const U &arg) const {
    return log_streamer<V>(make_val(arg), this);
  }

#if TRAA_CHECK_MSG_ENABLED
  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE static void call(const char *file, const int line,
                                                      const char *message, const Us &...args) {
    static constexpr check_arg_type t[] = {Us::Type()..., check_arg_type::t_end};
    fatal_log(file, line, message, t, args.get_val()...);
  }

  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE static void
  call_check_op(const char *file, const int line, const char *message, const Us &...args) {
    static constexpr check_arg_type t[] = {check_arg_type::t_check_op, Us::Type()...,
                                           check_arg_type::t_end};
    fatal_log(file, line, message, t, args.get_val()...);
  }
#else
  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE static void call(const char *file, const int line) {
    fatal_log(file, line);
  }
#endif
};

// Inductive case: We've already seen at least one << argument. The most recent
// one had type `T`, and the earlier ones had types `Ts`.
template <typename T, typename... Ts> class log_streamer<T, Ts...> final {
public:
  HEDLEY_ALWAYS_INLINE log_streamer(T arg, const log_streamer<Ts...> *prior)
      : arg_(arg), prior_(prior) {}

  template <typename U, typename V = decltype(make_val(std::declval<U>())),
            std::enable_if_t<std::is_arithmetic<U>::value || std::is_enum<U>::value> * = nullptr>
  HEDLEY_ALWAYS_INLINE log_streamer<V, T, Ts...> operator<<(U arg) const {
    return log_streamer<V, T, Ts...>(make_val(arg), this);
  }

  template <typename U, typename V = decltype(make_val(std::declval<U>())),
            std::enable_if_t<!std::is_arithmetic<U>::value && !std::is_enum<U>::value> * = nullptr>
  HEDLEY_ALWAYS_INLINE log_streamer<V, T, Ts...> operator<<(const U &arg) const {
    return log_streamer<V, T, Ts...>(make_val(arg), this);
  }

#if TRAA_CHECK_MSG_ENABLED
  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE void call(const char *file, const int line,
                                               const char *message, const Us &...args) const {
    prior_->call(file, line, message, arg_, args...);
  }

  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE void
  call_check_op(const char *file, const int line, const char *message, const Us &...args) const {
    prior_->call_check_op(file, line, message, arg_, args...);
  }
#else
  template <typename... Us>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE void call(const char *file, const int line) const {
    prior_->call(file, line);
  }
#endif

private:
  // The most recent argument.
  T arg_;

  // Earlier arguments.
  const log_streamer<Ts...> *prior_;
};

template <bool is_check_op> class fatal_log_call final {
public:
  fatal_log_call(const char *file, int line, const char *message)
      : file_(file), line_(line), message_(message) {}

  // This can be any binary operator with precedence lower than <<.
  template <typename... Ts>
  TRAA_NORETURN HEDLEY_ALWAYS_INLINE void operator&(const log_streamer<Ts...> &streamer) {
#if TRAA_CHECK_MSG_ENABLED
    is_check_op ? streamer.call_check_op(file_, line_, message_)
                : streamer.call(file_, line_, message_);
#else
    streamer.call(file_, line_);
#endif
  }

private:
  const char *file_;
  int line_;
  const char *message_;
};

#if TRAA_DCHECK_IS_ON

// Be helpful, and include file and line in the TRAA_CHECK_NOTREACHED error
// message.
#define TRAA_UNREACHABLE_FILE_AND_LINE_CALL_ARGS __FILE__, __LINE__
TRAA_NORETURN void unreachable_code_reached(const char *file, int line);

#else

// Be mindful of binary size, and don't include file and line in the
// TRAA_CHECK_NOTREACHED error message.
#define TRAA_UNREACHABLE_FILE_AND_LINE_CALL_ARGS
TRAA_NORETURN void unreachable_code_reached();

#endif

} // namespace checks_impl

// The actual stream used isn't important. We reference `ignored` in the code
// but don't evaluate it; this is to avoid "unused variable" warnings (we do so
// in a particularly convoluted way with an extra ?: because that appears to be
// the simplest construct that keeps Visual Studio from complaining about
// condition being unused).
#define TRAA_EAT_STREAM_PARAMETERS(ignored)                                                        \
  (true ? true : ((void)(ignored), true))                                                          \
      ? static_cast<void>(0)                                                                       \
      : traa::base::checks_impl::fatal_log_call<false>("", 0, "") &                                \
            traa::base::checks_impl::log_streamer<>()

// call TRAA_EAT_STREAM_PARAMETERS with an argument that fails to compile if
// values of the same types as `a` and `b` can't be compared with the given
// operation, and that would evaluate `a` and `b` if evaluated.
#define TRAA_EAT_STREAM_PARAMETERS_OP(op, a, b)                                                    \
  TRAA_EAT_STREAM_PARAMETERS(((void)traa::base::safe_##op(a, b)))

// TRAA_CHECK dies with a fatal error if condition is not true. It is *not*
// controlled by NDEBUG or anything else, so the check will be executed
// regardless of compilation mode.
//
// We make sure TRAA_CHECK et al. always evaluates `condition`, as
// doing TRAA_CHECK(FunctionWithSideEffect()) is a common idiom.
//
// TRAA_CHECK_OP is a helper macro for binary operators.
// Don't use this macro directly in your code, use TRAA_CHECK_EQ et al below.
#if TRAA_CHECK_MSG_ENABLED
#define TRAA_CHECK(condition)                                                                      \
  (condition) ? static_cast<void>(0)                                                               \
              : traa::base::checks_impl::fatal_log_call<false>(__FILE__, __LINE__, #condition) &   \
                    traa::base::checks_impl::log_streamer<>()

#define TRAA_CHECK_OP(name, op, val1, val2)                                                        \
  traa::base::safe_##name((val1), (val2))                                                          \
      ? static_cast<void>(0)                                                                       \
      : traa::base::checks_impl::fatal_log_call<true>(__FILE__, __LINE__,                          \
                                                      #val1 " " #op " " #val2) &                   \
            traa::base::checks_impl::log_streamer<>() << (val1) << (val2)
#else
#define TRAA_CHECK(condition)                                                                      \
  (condition) ? static_cast<void>(0)                                                               \
  : true      ? traa::base::checks_impl::fatal_log_call<false>(__FILE__, __LINE__, "") &           \
               traa::base::checks_impl::log_streamer<>()                                           \
         : traa::base::checks_impl::fatal_log_call<false>("", 0, "") &                             \
               traa::base::checks_impl::log_streamer<>()

#define TRAA_CHECK_OP(name, op, val1, val2)                                                        \
  traa::base::safe_##name((val1), (val2)) ? static_cast<void>(0)                                   \
  : true ? traa::base::checks_impl::fatal_log_call<true>(__FILE__, __LINE__, "") &                 \
               traa::base::checks_impl::log_streamer<>()                                           \
         : traa::base::checks_impl::fatal_log_call<false>("", 0, "") &                             \
               traa::base::checks_impl::log_streamer<>()
#endif

#define TRAA_CHECK_EQ(val1, val2) TRAA_CHECK_OP(eq, ==, val1, val2)
#define TRAA_CHECK_NE(val1, val2) TRAA_CHECK_OP(ne, !=, val1, val2)
#define TRAA_CHECK_LE(val1, val2) TRAA_CHECK_OP(le, <=, val1, val2)
#define TRAA_CHECK_LT(val1, val2) TRAA_CHECK_OP(lt, <, val1, val2)
#define TRAA_CHECK_GE(val1, val2) TRAA_CHECK_OP(ge, >=, val1, val2)
#define TRAA_CHECK_GT(val1, val2) TRAA_CHECK_OP(gt, >, val1, val2)

// The TRAA_DCHECK macro is equivalent to TRAA_CHECK except that it only generates
// code in debug builds. It does reference the condition parameter in all cases,
// though, so callers won't risk getting warnings about unused variables.
#if TRAA_DCHECK_IS_ON
#define TRAA_DCHECK(condition) TRAA_CHECK(condition)
#define TRAA_DCHECK_EQ(v1, v2) TRAA_CHECK_EQ(v1, v2)
#define TRAA_DCHECK_NE(v1, v2) TRAA_CHECK_NE(v1, v2)
#define TRAA_DCHECK_LE(v1, v2) TRAA_CHECK_LE(v1, v2)
#define TRAA_DCHECK_LT(v1, v2) TRAA_CHECK_LT(v1, v2)
#define TRAA_DCHECK_GE(v1, v2) TRAA_CHECK_GE(v1, v2)
#define TRAA_DCHECK_GT(v1, v2) TRAA_CHECK_GT(v1, v2)
#else
#define TRAA_DCHECK(condition) TRAA_EAT_STREAM_PARAMETERS(condition)
#define TRAA_DCHECK_EQ(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(eq, v1, v2)
#define TRAA_DCHECK_NE(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(ne, v1, v2)
#define TRAA_DCHECK_LE(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(le, v1, v2)
#define TRAA_DCHECK_LT(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(lt, v1, v2)
#define TRAA_DCHECK_GE(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(ge, v1, v2)
#define TRAA_DCHECK_GT(v1, v2) TRAA_EAT_STREAM_PARAMETERS_OP(gt, v1, v2)
#endif

#define TRAA_UNREACHABLE_CODE_HIT false
#define TRAA_DCHECK_NOTREACHED() TRAA_DCHECK(TRAA_UNREACHABLE_CODE_HIT)

// Kills the process with an error message. Never returns. Use when you wish to
// assert that a point in the code is never reached.
#define TRAA_CHECK_NOTREACHED()                                                                    \
  do {                                                                                             \
    traa::base::checks_impl::unreachable_code_reached(TRAA_UNREACHABLE_FILE_AND_LINE_CALL_ARGS);   \
  } while (0)

#define TRAA_FATAL()                                                                               \
  traa::base::checks_impl::fatal_log_call<false>(__FILE__, __LINE__, "FATAL()") &                  \
      traa::base::checks_impl::log_streamer<>()

// Performs the integer division a/b and returns the result. CHECKs that the
// remainder is zero.
template <typename T> inline T checked_div_exact(T a, T b) {
  TRAA_CHECK_EQ(a % b, 0) << a << " is not evenly divisible by " << b;
  return a / b;
}

} // namespace base
} // namespace traa

#else // __cplusplus not defined
// C version. Lacks many features compared to the C++ version, but usage
// guidelines are the same.

#define TRAA_CHECK(condition)                                                                      \
  do {                                                                                             \
    if (!(condition)) {                                                                            \
      fatal_message(__FILE__, __LINE__, TRAA_CHECK_EVAL_MESSAGE("check failed: " #condition));     \
    }                                                                                              \
  } while (0)

#define TRAA_CHECK_EQ(a, b) TRAA_CHECK((a) == (b))
#define TRAA_CHECK_NE(a, b) TRAA_CHECK((a) != (b))
#define TRAA_CHECK_LE(a, b) TRAA_CHECK((a) <= (b))
#define TRAA_CHECK_LT(a, b) TRAA_CHECK((a) < (b))
#define TRAA_CHECK_GE(a, b) TRAA_CHECK((a) >= (b))
#define TRAA_CHECK_GT(a, b) TRAA_CHECK((a) > (b))

#define TRAA_DCHECK(condition)                                                                     \
  do {                                                                                             \
    if (TRAA_DCHECK_IS_ON && !(condition)) {                                                       \
      fatal_message(__FILE__, __LINE__, TRAA_CHECK_EVAL_MESSAGE("DCHECK failed: " #condition));    \
    }                                                                                              \
  } while (0)

#define TRAA_DCHECK_EQ(a, b) TRAA_DCHECK((a) == (b))
#define TRAA_DCHECK_NE(a, b) TRAA_DCHECK((a) != (b))
#define TRAA_DCHECK_LE(a, b) TRAA_DCHECK((a) <= (b))
#define TRAA_DCHECK_LT(a, b) TRAA_DCHECK((a) < (b))
#define TRAA_DCHECK_GE(a, b) TRAA_DCHECK((a) >= (b))
#define TRAA_DCHECK_GT(a, b) TRAA_DCHECK((a) > (b))

#endif // __cplusplus

#endif // TRAA_BASE_CHECKS_H_
