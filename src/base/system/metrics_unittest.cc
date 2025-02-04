/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/platform.h"
#include "base/system/metrics.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;

#if defined(TRAA_METRICS_ENABLED)

namespace traa {
namespace base {

namespace {
const int k_sample = 22;

void add_sparse_sample(std::string_view name, int sample) {
  TRAA_HISTOGRAM_COUNTS_SPARSE_100(name, sample);
}
void add_sample_with_varying_name(int index, std::string_view name, int sample) {
  TRAA_HISTOGRAMS_COUNTS_100(index, name, sample);
}
} // namespace

class metrics_test : public ::testing::Test {
public:
  metrics_test() {}

protected:
  void SetUp() override { metrics::reset(); }
};

TEST_F(metrics_test, i_initially_no_samples) {
  EXPECT_EQ(0, metrics::num_samples("NonExisting"));
  EXPECT_EQ(0, metrics::num_events("NonExisting", k_sample));
  EXPECT_THAT(metrics::samples("NonExisting"), IsEmpty());
}

TEST_F(metrics_test, rtc_histogram_percentage_add_sample) {
  const std::string k_name = "Percentage";
  TRAA_HISTOGRAM_PERCENTAGE(k_name, k_sample);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, k_sample));
  EXPECT_THAT(metrics::samples(k_name), ElementsAre(Pair(k_sample, 1)));
}

TEST_F(metrics_test, rtc_histogram_enumeration_add_sample) {
  const std::string k_name = "Enumeration";
  TRAA_HISTOGRAM_ENUMERATION(k_name, k_sample, k_sample + 1);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, k_sample));
  EXPECT_THAT(metrics::samples(k_name), ElementsAre(Pair(k_sample, 1)));
}

TEST_F(metrics_test, rtc_histogram_boolean_add_sample) {
  const std::string k_name = "Boolean";
  const int k_sample = 0;
  TRAA_HISTOGRAM_BOOLEAN(k_name, k_sample);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, k_sample));
  EXPECT_THAT(metrics::samples(k_name), ElementsAre(Pair(k_sample, 1)));
}

TEST_F(metrics_test, rtc_histogram_counts_sparse_add_sample) {
  const std::string k_name = "CountsSparse100";
  TRAA_HISTOGRAM_COUNTS_SPARSE_100(k_name, k_sample);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, k_sample));
  EXPECT_THAT(metrics::samples(k_name), ElementsAre(Pair(k_sample, 1)));
}

TEST_F(metrics_test, rtc_histogram_counts_add_sample) {
  const std::string k_name = "Counts100";
  TRAA_HISTOGRAM_COUNTS_100(k_name, k_sample);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, k_sample));
  EXPECT_THAT(metrics::samples(k_name), ElementsAre(Pair(k_sample, 1)));
}

TEST_F(metrics_test, rtc_histogram_counts_add_multiple_samples) {
  const std::string k_name = "Counts200";
  const int kNumSamples = 10;
  std::map<int, int> samples;
  for (int i = 1; i <= kNumSamples; ++i) {
    TRAA_HISTOGRAM_COUNTS_200(k_name, i);
    EXPECT_EQ(1, metrics::num_events(k_name, i));
    EXPECT_EQ(i, metrics::num_samples(k_name));
    samples[i] = 1;
  }
  EXPECT_EQ(samples, metrics::samples(k_name));
}

TEST_F(metrics_test, rtc_histograms_counts_add_sample) {
  add_sample_with_varying_name(0, "Name1", k_sample);
  add_sample_with_varying_name(1, "Name2", k_sample + 1);
  add_sample_with_varying_name(2, "Name3", k_sample + 2);
  EXPECT_EQ(1, metrics::num_samples("Name1"));
  EXPECT_EQ(1, metrics::num_samples("Name2"));
  EXPECT_EQ(1, metrics::num_samples("Name3"));
  EXPECT_EQ(1, metrics::num_events("Name1", k_sample + 0));
  EXPECT_EQ(1, metrics::num_events("Name2", k_sample + 1));
  EXPECT_EQ(1, metrics::num_events("Name3", k_sample + 2));
  EXPECT_THAT(metrics::samples("Name1"), ElementsAre(Pair(k_sample + 0, 1)));
  EXPECT_THAT(metrics::samples("Name2"), ElementsAre(Pair(k_sample + 1, 1)));
  EXPECT_THAT(metrics::samples("Name3"), ElementsAre(Pair(k_sample + 2, 1)));
}

#if TRAA_DCHECK_IS_ON && GTEST_HAS_DEATH_TEST && !defined(TRAA_OS_LINUX_ANDROID)
using metrics_death_test = metrics_test;
TEST_F(metrics_death_test, rtc_histograms_counts_invalid_index) {
  EXPECT_DEATH(TRAA_HISTOGRAMS_COUNTS_1000(-1, "Name", k_sample), "");
  EXPECT_DEATH(TRAA_HISTOGRAMS_COUNTS_1000(3, "Name", k_sample), "");
  EXPECT_DEATH(TRAA_HISTOGRAMS_COUNTS_1000(3u, "Name", k_sample), "");
}
#endif

TEST_F(metrics_test, rtc_histogram_sparse_non_constant_name_works) {
  add_sparse_sample("Sparse1", k_sample);
  add_sparse_sample("Sparse2", k_sample);
  EXPECT_EQ(1, metrics::num_samples("Sparse1"));
  EXPECT_EQ(1, metrics::num_samples("Sparse2"));
  EXPECT_THAT(metrics::samples("Sparse1"), ElementsAre(Pair(k_sample, 1)));
  EXPECT_THAT(metrics::samples("Sparse2"), ElementsAre(Pair(k_sample, 1)));
}

} // namespace base
} // namespace traa

#endif // TRAA_METRICS_ENABLED