/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "base/random.h"

#include <math.h>

namespace traa {
namespace base {

random::random(uint64_t seed) { state_ = seed; }

uint32_t random::rand(uint32_t t) {
  // Casting the output to 32 bits will give an almost uniform number.
  // Pr[x=0] = (2^32-1) / (2^64-1)
  // Pr[x=k] = 2^32 / (2^64-1) for k!=0
  // Uniform would be Pr[x=k] = 2^32 / 2^64 for all 32-bit integers k.
  uint32_t x = static_cast<uint32_t>(next_output());
  // If x / 2^32 is uniform on [0,1), then x / 2^32 * (t+1) is uniform on
  // the interval [0,t+1), so the integer part is uniform on [0,t].
  uint64_t result = x * (static_cast<uint64_t>(t) + 1);
  result >>= 32;
  return static_cast<uint32_t>(result);
}

uint32_t random::rand(uint32_t low, uint32_t high) { return rand(high - low) + low; }

int32_t random::rand(int32_t low, int32_t high) {
  const int64_t low_i64{low};
  return static_cast<int32_t>(rand(static_cast<uint32_t>(high - low_i64)) + low_i64);
}

template <> float random::rand<float>() {
  double result = static_cast<double>(next_output()) - 1;
  result = result / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
  return static_cast<float>(result);
}

template <> double random::rand<double>() {
  double result = static_cast<double>(next_output()) - 1;
  result = result / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
  return result;
}

template <> bool random::rand<bool>() { return rand(0, 1) == 1; }

double random::gaussian(double mean, double standard_deviation) {
  // Creating a Normal distribution variable from two independent uniform
  // variables based on the Box-Muller transform, which is defined on the
  // interval (0, 1]. Note that we rely on next_output to generate integers
  // in the range [1, 2^64-1]. Normally this behavior is a bit frustrating,
  // but here it is exactly what we need.
  constexpr double k_pi = 3.14159265358979323846;
  double u1 = static_cast<double>(next_output()) / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
  double u2 = static_cast<double>(next_output()) / static_cast<double>(0xFFFFFFFFFFFFFFFFull);
  return mean + standard_deviation * sqrt(-2 * log(u1)) * cos(2 * k_pi * u2);
}

double random::exponential(double lambda) {
  double uniform = rand<double>();
  return -log(uniform) / lambda;
}

} // namespace base
} // namespace traa
