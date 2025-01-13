//
// Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
//

#ifndef TRAA_BASE_SYSTEM_METRICS_H_
#define TRAA_BASE_SYSTEM_METRICS_H_

#include "base/checks.h"
#include "base/string_utils.h"

#include <stddef.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace traa {
namespace base {
namespace metrics_impl {
template <typename... Ts> void no_op(const Ts &...) {}
} // namespace metrics_impl
} // namespace base
} // namespace traa

#if defined(TRAA_METRICS_ENABLED)
#define EXPECT_METRIC_EQ(val1, val2) EXPECT_EQ(val1, val2)
#define EXPECT_METRIC_EQ_WAIT(val1, val2, timeout) EXPECT_EQ_WAIT(val1, val2, timeout)
#define EXPECT_METRIC_GT(val1, val2) EXPECT_GT(val1, val2)
#define EXPECT_METRIC_LE(val1, val2) EXPECT_LE(val1, val2)
#define EXPECT_METRIC_TRUE(conditon) EXPECT_TRUE(conditon)
#define EXPECT_METRIC_FALSE(conditon) EXPECT_FALSE(conditon)
#define EXPECT_METRIC_THAT(value, matcher) EXPECT_THAT(value, matcher)
#else
#define EXPECT_METRIC_EQ(val1, val2) traa::base::metrics_impl::no_op(val1, val2)
#define EXPECT_METRIC_EQ_WAIT(val1, val2, timeout)                                                 \
  traa::base::metrics_impl::no_op(val1, val2, timeout)
#define EXPECT_METRIC_GT(val1, val2) traa::base::metrics_impl::no_op(val1, val2)
#define EXPECT_METRIC_LE(val1, val2) traa::base::metrics_impl::no_op(val1, val2)
#define EXPECT_METRIC_TRUE(condition) traa::base::metrics_impl::no_op(condition || true)
#define EXPECT_METRIC_FALSE(condition) traa::base::metrics_impl::no_op(condition && false)
#define EXPECT_METRIC_THAT(value, matcher) traa::base::metrics_impl::no_op(value, testing::_)
#endif

#if defined(TRAA_METRICS_ENABLED)
// Macros for allowing WebRTC clients (e.g. Chrome) to gather and aggregate
// statistics.
//
// Histogram for counters.
// TRAA_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count);
//
// Histogram for enumerators.
// The boundary should be above the max enumerator sample.
// TRAA_HISTOGRAM_ENUMERATION(name, sample, boundary);
//
//
// The macros use the methods histogram_factory_get_counts,
// histogram_factory_get_enumeration and histogram_add.
//
// By default WebRTC provides implementations of the aforementioned methods
// that can be found in system_wrappers/source/metrics.cc. If clients want to
// provide a custom version, they will have to:
//
// 1. Compile WebRTC defining the preprocessor macro
//    TRAA_EXCLUDE_METRICS_DEFAULT (if GN is used this can be achieved
//    by setting the GN arg rtc_exclude_metrics_default to true).
// 2. Provide implementations of:
//    histogram* traa::base::metrics::histogram_factory_get_counts(
//        std::string_view name, int sample, int min, int max,
//        int bucket_count);
//    histogram* traa::base::metrics::histogram_factory_get_enumeration(
//        std::string_view name, int sample, int boundary);
//    void traa::base::metrics::histogram_add(
//        histogram* histogram_pointer, std::string_view name, int sample);
//
// Example usage:
//
// TRAA_HISTOGRAM_COUNTS("WebRTC.Video.NacksSent", nacks_sent, 1, 100000, 100);
//
// enum Types {
//   kTypeX,
//   kTypeY,
//   kBoundary,
// };
//
// TRAA_HISTOGRAM_ENUMERATION("WebRTC.Types", kTypeX, kBoundary);
//
// NOTE: It is recommended to do the Chromium review for modifications to
// histograms.xml before new metrics are committed to WebRTC.

// Macros for adding samples to a named histogram.

// Histogram for counters (exponentially spaced buckets).
#define TRAA_HISTOGRAM_COUNTS_100(name, sample) TRAA_HISTOGRAM_COUNTS(name, sample, 1, 100, 50)

#define TRAA_HISTOGRAM_COUNTS_200(name, sample) TRAA_HISTOGRAM_COUNTS(name, sample, 1, 200, 50)

#define TRAA_HISTOGRAM_COUNTS_500(name, sample) TRAA_HISTOGRAM_COUNTS(name, sample, 1, 500, 50)

#define TRAA_HISTOGRAM_COUNTS_1000(name, sample) TRAA_HISTOGRAM_COUNTS(name, sample, 1, 1000, 50)

#define TRAA_HISTOGRAM_COUNTS_10000(name, sample) TRAA_HISTOGRAM_COUNTS(name, sample, 1, 10000, 50)

#define TRAA_HISTOGRAM_COUNTS_100000(name, sample)                                                 \
  TRAA_HISTOGRAM_COUNTS(name, sample, 1, 100000, 50)

#define TRAA_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count)                                \
  TRAA_HISTOGRAM_COMMON_BLOCK(                                                                     \
      name, sample,                                                                                \
      traa::base::metrics::histogram_factory_get_counts(name, min, max, bucket_count))

#define TRAA_HISTOGRAM_COUNTS_LINEAR(name, sample, min, max, bucket_count)                         \
  TRAA_HISTOGRAM_COMMON_BLOCK(                                                                     \
      name, sample,                                                                                \
      traa::base::metrics::histogram_factory_get_counts_linear(name, min, max, bucket_count))

// Slow metrics: pointer to metric is acquired at each call and is not cached.
//
#define TRAA_HISTOGRAM_COUNTS_SPARSE_100(name, sample)                                             \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 100, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_200(name, sample)                                             \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 200, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_500(name, sample)                                             \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 500, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_1000(name, sample)                                            \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 1000, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_10000(name, sample)                                           \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 10000, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_100000(name, sample)                                          \
  TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, 1, 100000, 50)

#define TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, min, max, bucket_count)                         \
  TRAA_HISTOGRAM_COMMON_BLOCK_SLOW(                                                                \
      name, sample,                                                                                \
      traa::base::metrics::histogram_factory_get_counts(name, min, max, bucket_count))

// Histogram for percentage (evenly spaced buckets).
#define TRAA_HISTOGRAM_PERCENTAGE_SPARSE(name, sample)                                             \
  TRAA_HISTOGRAM_ENUMERATION_SPARSE(name, sample, 101)

// Histogram for booleans.
#define TRAA_HISTOGRAM_BOOLEAN_SPARSE(name, sample)                                                \
  TRAA_HISTOGRAM_ENUMERATION_SPARSE(name, sample, 2)

// Histogram for enumerators (evenly spaced buckets).
// `boundary` should be above the max enumerator sample.
//
// TODO(qingsi): Refactor the default implementation given by traa_histogram,
// which is already sparse, and remove the boundary argument from the macro.
#define TRAA_HISTOGRAM_ENUMERATION_SPARSE(name, sample, boundary)                                  \
  TRAA_HISTOGRAM_COMMON_BLOCK_SLOW(                                                                \
      name, sample, traa::base::metrics::sparse_histogram_factory_get_enumeration(name, boundary))

// Histogram for percentage (evenly spaced buckets).
#define TRAA_HISTOGRAM_PERCENTAGE(name, sample) TRAA_HISTOGRAM_ENUMERATION(name, sample, 101)

// Histogram for booleans.
#define TRAA_HISTOGRAM_BOOLEAN(name, sample) TRAA_HISTOGRAM_ENUMERATION(name, sample, 2)

// Histogram for enumerators (evenly spaced buckets).
// `boundary` should be above the max enumerator sample.
#define TRAA_HISTOGRAM_ENUMERATION(name, sample, boundary)                                         \
  TRAA_HISTOGRAM_COMMON_BLOCK_SLOW(                                                                \
      name, sample, traa::base::metrics::histogram_factory_get_enumeration(name, boundary))

// The name of the histogram should not vary.
#define TRAA_HISTOGRAM_COMMON_BLOCK(constant_name, sample, factory_get_invocation)                 \
  do {                                                                                             \
    static std::atomic<traa::base::metrics::histogram *> atomic_histogram_pointer(nullptr);        \
    traa::base::metrics::histogram *histogram_pointer =                                            \
        atomic_histogram_pointer.load(std::memory_order_acquire);                                  \
    if (!histogram_pointer) {                                                                      \
      histogram_pointer = factory_get_invocation;                                                  \
      traa::base::metrics::histogram *null_histogram = nullptr;                                    \
      atomic_histogram_pointer.compare_exchange_strong(null_histogram, histogram_pointer);         \
    }                                                                                              \
    if (histogram_pointer) {                                                                       \
      traa::base::metrics::histogram_add(histogram_pointer, sample);                               \
    }                                                                                              \
  } while (0)

// The histogram is constructed/found for each call.
// May be used for histograms with infrequent updates.`
#define TRAA_HISTOGRAM_COMMON_BLOCK_SLOW(name, sample, factory_get_invocation)                     \
  do {                                                                                             \
    traa::base::metrics::histogram *histogram_pointer = factory_get_invocation;                    \
    if (histogram_pointer) {                                                                       \
      traa::base::metrics::histogram_add(histogram_pointer, sample);                               \
    }                                                                                              \
  } while (0)

// Helper macros.
// Macros for calling a histogram with varying name (e.g. when using a metric
// in different modes such as real-time vs screenshare). Fast, because pointer
// is cached. `index` should be different for different names. Allowed `index`
// values are 0, 1, and 2.
#define TRAA_HISTOGRAMS_COUNTS_100(index, name, sample)                                            \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 100, 50))

#define TRAA_HISTOGRAMS_COUNTS_200(index, name, sample)                                            \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 200, 50))

#define TRAA_HISTOGRAMS_COUNTS_500(index, name, sample)                                            \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 500, 50))

#define TRAA_HISTOGRAMS_COUNTS_1000(index, name, sample)                                           \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 1000, 50))

#define TRAA_HISTOGRAMS_COUNTS_10000(index, name, sample)                                          \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 10000, 50))

#define TRAA_HISTOGRAMS_COUNTS_100000(index, name, sample)                                         \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_COUNTS(name, sample, 1, 100000, 50))

#define TRAA_HISTOGRAMS_ENUMERATION(index, name, sample, boundary)                                 \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_ENUMERATION(name, sample, boundary))

#define TRAA_HISTOGRAMS_PERCENTAGE(index, name, sample)                                            \
  TRAA_HISTOGRAMS_COMMON(index, name, sample, TRAA_HISTOGRAM_PERCENTAGE(name, sample))

#define TRAA_HISTOGRAMS_COMMON(index, name, sample, macro_invocation)                              \
  do {                                                                                             \
    switch (index) {                                                                               \
    case 0:                                                                                        \
      macro_invocation;                                                                            \
      break;                                                                                       \
    case 1:                                                                                        \
      macro_invocation;                                                                            \
      break;                                                                                       \
    case 2:                                                                                        \
      macro_invocation;                                                                            \
      break;                                                                                       \
    default:                                                                                       \
      TRAA_DCHECK_NOTREACHED();                                                                    \
    }                                                                                              \
  } while (0)

#else

////////////////////////////////////////////////////////////////////////////////
// This section defines no-op alternatives to the metrics macros when
// TRAA_METRICS_ENABLED is defined.

#define TRAA_HISTOGRAM_COUNTS_100(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_200(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_500(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_1000(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_10000(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_100000(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count)                                \
  traa::base::metrics_impl::no_op(name, sample, min, max, bucket_count)

#define TRAA_HISTOGRAM_COUNTS_LINEAR(name, sample, min, max, bucket_count)                         \
  traa::base::metrics_impl::no_op(name, sample, min, max, bucket_count)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_100(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_200(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_500(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_1000(name, sample)                                            \
  traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_10000(name, sample)                                           \
  traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE_100000(name, sample)                                          \
  traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_COUNTS_SPARSE(name, sample, min, max, bucket_count)                         \
  traa::base::metrics_impl::no_op(name, sample, min, max, bucket_count)

#define TRAA_HISTOGRAM_PERCENTAGE_SPARSE(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_BOOLEAN_SPARSE(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_ENUMERATION_SPARSE(name, sample, boundary)                                  \
  traa::base::metrics_impl::no_op(name, sample, boundary)

#define TRAA_HISTOGRAM_PERCENTAGE(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_BOOLEAN(name, sample) traa::base::metrics_impl::no_op(name, sample)

#define TRAA_HISTOGRAM_ENUMERATION(name, sample, boundary)                                         \
  traa::base::metrics_impl::no_op(name, sample, boundary)

#define TRAA_HISTOGRAM_COMMON_BLOCK(constant_name, sample, factory_get_invocation)                 \
  traa::base::metrics_impl::no_op(constant_name, sample, factory_get_invocation)

#define TRAA_HISTOGRAM_COMMON_BLOCK_SLOW(name, sample, factory_get_invocation)                     \
  traa::base::metrics_impl::no_op(name, sample, factory_get_invocation)

#define TRAA_HISTOGRAMS_COUNTS_100(index, name, sample)                                            \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COUNTS_200(index, name, sample)                                            \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COUNTS_500(index, name, sample)                                            \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COUNTS_1000(index, name, sample)                                           \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COUNTS_10000(index, name, sample)                                          \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COUNTS_100000(index, name, sample)                                         \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_ENUMERATION(index, name, sample, boundary)                                 \
  traa::base::metrics_impl::no_op(index, name, sample, boundary)

#define TRAA_HISTOGRAMS_PERCENTAGE(index, name, sample)                                            \
  traa::base::metrics_impl::no_op(index, name, sample)

#define TRAA_HISTOGRAMS_COMMON(index, name, sample, macro_invocation)                              \
  traa::base::metrics_impl::no_op(index, name, sample, macro_invocation)

#endif // TRAA_METRICS_ENABLED

namespace traa {
namespace base {

namespace metrics {

// Time that should have elapsed for stats that are gathered once per call.
constexpr int k_min_run_time_in_seconds = 10;

class histogram;

// Functions for getting pointer to histogram (constructs or finds the named
// histogram).

// Get histogram for counters.
histogram *histogram_factory_get_counts(std::string_view name, int min, int max, int bucket_count);

// Get histogram for counters with linear bucket spacing.
histogram *histogram_factory_get_counts_linear(std::string_view name, int min, int max,
                                               int bucket_count);

// Get histogram for enumerators.
// `boundary` should be above the max enumerator sample.
histogram *histogram_factory_get_enumeration(std::string_view name, int boundary);

// Get sparse histogram for enumerators.
// `boundary` should be above the max enumerator sample.
histogram *sparse_histogram_factory_get_enumeration(std::string_view name, int boundary);

// Function for adding a `sample` to a histogram.
void histogram_add(histogram *histogram_pointer, int sample);

struct sample_info {
  sample_info(std::string_view name, int min, int max, size_t bucket_count);
  ~sample_info();

  const std::string name;
  const int min;
  const int max;
  const size_t bucket_count;
  std::map<int, int> samples; // <value, # of events>
};

// Enables collection of samples.
// This method should be called before any other call into webrtc.
void enable();

// Gets histograms and clears all samples.
void get_and_reset(
    std::map<std::string, std::unique_ptr<sample_info>, string_view_cmp> *histograms);

// Functions below are mainly for testing.

// Clears all samples.
void reset();

// Returns the number of times the `sample` has been added to the histogram.
int num_events(std::string_view name, int sample);

// Returns the total number of added samples to the histogram.
int num_samples(std::string_view name);

// Returns the minimum sample value (or -1 if the histogram has no samples).
int min_sample(std::string_view name);

// Returns a map with keys the samples with at least one event and values the
// number of events for that sample.
std::map<int, int> samples(std::string_view name);

} // namespace metrics

} // namespace base
} // namespace traa

#endif // TRAA_BASE_SYSTEM_METRICS_H_
