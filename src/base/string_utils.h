/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_STRING_UTILS_H_
#define TRAA_BASE_STRING_UTILS_H_

#include "base/platform.h"

#include <stdio.h>
#include <string.h>

#if defined(TRAA_OS_WINDOWS)
#include <malloc.h>
#include <wchar.h>
#include <windows.h>

#endif // TRAA_OS_WINDOWS

#if defined(TRAA_OS_POSIX)
#include <stdlib.h>
#include <strings.h>
#endif // TRAA_OS_POSIX

#include <string>
#include <string_view>

namespace traa {
namespace base {

const size_t SIZE_UNKNOWN = static_cast<size_t>(-1);

// An std::string_view comparator functor for use with container types such as
// std::map that support heterogenous lookup.
//
// Example usage:
// std::map<std::string, int, traa::base::string_view_cmp> my_map;
struct string_view_cmp {
  using is_transparent = void;
  bool operator()(std::string_view a, std::string_view b) const { return a < b; }
};

// Safe version of strncpy that always nul-terminate.
size_t strcpyn(char *buffer, size_t buflen, std::string_view source);

///////////////////////////////////////////////////////////////////////////////
// UTF helpers (Windows only)
///////////////////////////////////////////////////////////////////////////////

#if defined(TRAA_OS_WINDOWS)

inline std::wstring to_utf16(const char *utf8, size_t len) {
  if (len == 0)
    return std::wstring();
  int len16 = ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len), nullptr, 0);
  std::wstring ws(len16, 0);
  ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len), &*ws.begin(), len16);
  return ws;
}

inline std::wstring to_utf16(std::string_view str) { return to_utf16(str.data(), str.length()); }

inline std::string to_utf8(const wchar_t *wide, size_t len) {
  if (len == 0)
    return std::string();
  int len8 =
      ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
  std::string ns(len8, 0);
  ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), &*ns.begin(), len8, nullptr,
                        nullptr);
  return ns;
}

inline std::string to_utf8(const wchar_t *wide) { return to_utf8(wide, wcslen(wide)); }

inline std::string to_utf8(const std::wstring &wstr) { return to_utf8(wstr.data(), wstr.length()); }

#endif // TRAA_OS_WINDOWS

// TODO(jonasolsson): replace with absl::Hex when that becomes available.
std::string to_hex(int i);

// compile_time_string comprises of a string-like object which can be used as a
// regular const char* in compile time and supports concatenation. Useful for
// concatenating constexpr strings in for example macro declarations.
namespace traa_base_string_utils_internal {
template <int NPlus1> struct compile_time_string {
  char string[NPlus1] = {0};
  constexpr compile_time_string() = default;
  template <int MPlus1> explicit constexpr compile_time_string(const char (&chars)[MPlus1]) {
    char *chars_pointer = string;
    for (auto c : chars)
      *chars_pointer++ = c;
  }
  template <int MPlus1> constexpr auto concat(compile_time_string<MPlus1> b) {
    compile_time_string<NPlus1 + MPlus1 - 1> result;
    char *chars_pointer = result.string;
    for (auto c : string)
      *chars_pointer++ = c;
    chars_pointer = result.string + NPlus1 - 1;
    for (auto c : b.string)
      *chars_pointer++ = c;
    result.string[NPlus1 + MPlus1 - 2] = 0;
    return result;
  }
  constexpr operator const char *() { return string; }
};
} // namespace traa_base_string_utils_internal

// Makes a constexpr compile_time_string<X> without having to specify X
// explicitly.
template <int N> constexpr auto make_compile_time_string(const char (&a)[N]) {
  return traa_base_string_utils_internal::compile_time_string<N>(a);
}

} // namespace base
} // namespace traa

#endif // TRAA_BASE_STRING_UTILS_H_
