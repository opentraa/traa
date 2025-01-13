/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/string_utils.h"
#include "base/system/metrics.h"

#include "base/checks.h"

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#if defined(TRAA_METRICS_ENABLED)

namespace traa {
namespace base {

namespace {
const int k_sample = 22;
const char k_name[] = "Name";

int num_samples(std::string_view name,
                const std::map<std::string, std::unique_ptr<metrics::sample_info>, string_view_cmp>
                    &histograms) {
  const auto it = histograms.find(name);
  if (it == histograms.end())
    return 0;

  int num_samples = 0;
  for (const auto &sample : it->second->samples)
    num_samples += sample.second;

  return num_samples;
}

int num_events(std::string_view name, int sample,
               const std::map<std::string, std::unique_ptr<metrics::sample_info>, string_view_cmp>
                   &histograms) {
  const auto it = histograms.find(name);
  if (it == histograms.end())
    return 0;

  const auto it_sample = it->second->samples.find(sample);
  if (it_sample == it->second->samples.end())
    return 0;

  return it_sample->second;
}
} // namespace

class metrics_default_test : public ::testing::Test {
public:
  metrics_default_test() {}

protected:
  void SetUp() override { metrics::reset(); }
};

TEST_F(metrics_default_test, reset) {
  TRAA_HISTOGRAM_PERCENTAGE(k_name, k_sample);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  metrics::reset();
  EXPECT_EQ(0, metrics::num_samples(k_name));
}

TEST_F(metrics_default_test, num_samples) {
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 5);
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 5);
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 10);
  EXPECT_EQ(3, metrics::num_samples(k_name));
  EXPECT_EQ(0, metrics::num_samples("NonExisting"));
}

TEST_F(metrics_default_test, num_events) {
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 5);
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 5);
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 10);
  EXPECT_EQ(2, metrics::num_events(k_name, 5));
  EXPECT_EQ(1, metrics::num_events(k_name, 10));
  EXPECT_EQ(0, metrics::num_events(k_name, 11));
  EXPECT_EQ(0, metrics::num_events("NonExisting", 5));
}

TEST_F(metrics_default_test, min_sample) {
  TRAA_HISTOGRAM_PERCENTAGE(k_name, k_sample);
  TRAA_HISTOGRAM_PERCENTAGE(k_name, k_sample + 1);
  EXPECT_EQ(k_sample, metrics::min_sample(k_name));
  EXPECT_EQ(-1, metrics::min_sample("NonExisting"));
}

TEST_F(metrics_default_test, overflow) {
  const std::string k_name = "Overflow";
  // Samples should end up in overflow bucket.
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 101);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, 101));
  TRAA_HISTOGRAM_PERCENTAGE(k_name, 102);
  EXPECT_EQ(2, metrics::num_samples(k_name));
  EXPECT_EQ(2, metrics::num_events(k_name, 101));
}

TEST_F(metrics_default_test, underflow) {
  const std::string k_name = "Underflow";
  // Samples should end up in underflow bucket.
  TRAA_HISTOGRAM_COUNTS_10000(k_name, 0);
  EXPECT_EQ(1, metrics::num_samples(k_name));
  EXPECT_EQ(1, metrics::num_events(k_name, 0));
  TRAA_HISTOGRAM_COUNTS_10000(k_name, -1);
  EXPECT_EQ(2, metrics::num_samples(k_name));
  EXPECT_EQ(2, metrics::num_events(k_name, 0));
}

TEST_F(metrics_default_test, get_and_reset) {
  std::map<std::string, std::unique_ptr<metrics::sample_info>, string_view_cmp> histograms;
  metrics::get_and_reset(&histograms);
  EXPECT_EQ(0u, histograms.size());
  TRAA_HISTOGRAM_PERCENTAGE("Histogram1", 4);
  TRAA_HISTOGRAM_PERCENTAGE("Histogram1", 5);
  TRAA_HISTOGRAM_PERCENTAGE("Histogram1", 5);
  TRAA_HISTOGRAM_PERCENTAGE("Histogram2", 10);
  EXPECT_EQ(3, metrics::num_samples("Histogram1"));
  EXPECT_EQ(1, metrics::num_samples("Histogram2"));

  metrics::get_and_reset(&histograms);
  EXPECT_EQ(2u, histograms.size());
  EXPECT_EQ(0, metrics::num_samples("Histogram1"));
  EXPECT_EQ(0, metrics::num_samples("Histogram2"));

  EXPECT_EQ(3, num_samples("Histogram1", histograms));
  EXPECT_EQ(1, num_samples("Histogram2", histograms));
  EXPECT_EQ(1, num_events("Histogram1", 4, histograms));
  EXPECT_EQ(2, num_events("Histogram1", 5, histograms));
  EXPECT_EQ(1, num_events("Histogram2", 10, histograms));

  // Add samples after reset.
  metrics::get_and_reset(&histograms);
  EXPECT_EQ(0u, histograms.size());
  TRAA_HISTOGRAM_PERCENTAGE("Histogram1", 50);
  TRAA_HISTOGRAM_PERCENTAGE("Histogram2", 8);
  EXPECT_EQ(1, metrics::num_samples("Histogram1"));
  EXPECT_EQ(1, metrics::num_samples("Histogram2"));
  EXPECT_EQ(1, metrics::num_events("Histogram1", 50));
  EXPECT_EQ(1, metrics::num_events("Histogram2", 8));
}

TEST_F(metrics_default_test, min_max_bucket) {
  const std::string k_name = "MinMaxCounts100";
  TRAA_HISTOGRAM_COUNTS_100(k_name, 4);

  std::map<std::string, std::unique_ptr<metrics::sample_info>, string_view_cmp> histograms;
  metrics::get_and_reset(&histograms);
  EXPECT_EQ(1u, histograms.size());
  EXPECT_EQ(k_name, histograms.begin()->second->name);
  EXPECT_EQ(1, histograms.begin()->second->min);
  EXPECT_EQ(100, histograms.begin()->second->max);
  EXPECT_EQ(50u, histograms.begin()->second->bucket_count);
  EXPECT_EQ(1u, histograms.begin()->second->samples.size());
}

} // namespace base
} // namespace traa

#endif // TRAA_METRICS_ENABLED
