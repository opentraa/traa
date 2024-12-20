/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_RANDOM_H_
#define TRAA_BASE_RANDOM_H_

#include <stdint.h>

#include <limits>

namespace traa {
namespace base {

class random {
public:
  // TODO(tommi): Change this so that the seed can be initialized internally,
  // e.g. by offering two ways of constructing or offer a static method that
  // returns a seed that's suitable for initialization.
  // The problem now is that callers are calling clock_->TimeInMicroseconds()
  // which calls TickTime::Now().Ticks(), which can return a very low value on
  // Mac and can result in a seed of 0 after conversion to microseconds.
  // Besides the quality of the random seed being poor, this also requires
  // the client to take on extra dependencies to generate a seed.
  // If we go for a static seed generator in Random, we can use something from
  // webrtc/rtc_base and make sure that it works the same way across platforms.
  // See also discussion here: https://codereview.webrtc.org/1623543002/
  explicit random(uint64_t seed);

  random() = delete;
  random(const random &) = delete;
  random &operator=(const random &) = delete;

  // Return pseudo-random integer of the specified type.
  // We need to limit the size to 32 bits to keep the output close to uniform.
  template <typename T> T rand() {
    static_assert(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::radix == 2 &&
                      std::numeric_limits<T>::digits <= 32,
                  "rand is only supported for built-in integer types that are "
                  "32 bits or smaller.");
    return static_cast<T>(next_output());
  }

  // Uniformly distributed pseudo-random number in the interval [0, t].
  uint32_t rand(uint32_t t);

  // Uniformly distributed pseudo-random number in the interval [low, high].
  uint32_t rand(uint32_t low, uint32_t high);

  // Uniformly distributed pseudo-random number in the interval [low, high].
  int32_t rand(int32_t low, int32_t high);

  // Normal Distribution.
  double gaussian(double mean, double standard_deviation);

  // Exponential Distribution.
  double exponential(double lambda);

private:
  // Outputs a nonzero 64-bit random number using Xorshift algorithm.
  // https://en.wikipedia.org/wiki/Xorshift
  uint64_t next_output() {
    state_ ^= state_ >> 12;
    state_ ^= state_ << 25;
    state_ ^= state_ >> 27;

    return state_ * 2685821657736338717ull;
  }

  uint64_t state_;
};

// Return pseudo-random number in the interval [0.0, 1.0).
template <> float random::rand<float>();

// Return pseudo-random number in the interval [0.0, 1.0).
template <> double random::rand<double>();

// Return pseudo-random boolean value.
template <> bool random::rand<bool>();

} // namespace base
} // namespace traa

#endif // TRAA_BASE_RANDOM_H_