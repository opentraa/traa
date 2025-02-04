// Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
//

#include "base/system/metrics.h"
#include "base/thread_annotations.h"

#include <algorithm>
#include <mutex>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

// Default implementation of histogram methods for WebRTC clients that do not
// want to provide their own implementation.

namespace traa {
namespace base {

namespace metrics {
class histogram;

namespace {
// Limit for the maximum number of sample values that can be stored.
// TODO(asapersson): Consider using bucket count (and set up
// linearly/exponentially spaced buckets) if samples are logged more frequently.
const int k_max_sample_map_size = 300;

class traa_histogram {
public:
  traa_histogram(std::string_view name, int min, int max, int bucket_count)
      : min_(min), max_(max), info_(name, min, max, bucket_count) {
    TRAA_DCHECK_GT(bucket_count, 0);
  }

  traa_histogram(const traa_histogram &) = delete;
  traa_histogram &operator=(const traa_histogram &) = delete;

  void add(int sample) {
    sample = std::min(sample, max_);
    sample = std::max(sample, min_ - 1); // Underflow bucket.

    std::lock_guard<std::mutex> lock(mutex_);
    if (info_.samples.size() == k_max_sample_map_size &&
        info_.samples.find(sample) == info_.samples.end()) {
      return;
    }
    ++info_.samples[sample];
  }

  // Returns a copy (or nullptr if there are no samples) and clears samples.
  std::unique_ptr<sample_info> get_and_reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (info_.samples.empty())
      return nullptr;

    sample_info *copy = new sample_info(info_.name, info_.min, info_.max, info_.bucket_count);

    std::swap(info_.samples, copy->samples);

    return std::unique_ptr<sample_info>(copy);
  }

  const std::string &name() const { return info_.name; }

  // Functions only for testing.
  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    info_.samples.clear();
  }

  int num_events(int sample) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = info_.samples.find(sample);
    return (it == info_.samples.end()) ? 0 : it->second;
  }

  int num_samples() const {
    int num_samples = 0;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &sample : info_.samples) {
      num_samples += sample.second;
    }
    return num_samples;
  }

  int min_sample() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (info_.samples.empty()) ? -1 : info_.samples.begin()->first;
  }

  std::map<int, int> samples() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_.samples;
  }

private:
  mutable std::mutex mutex_;
  const int min_;
  const int max_;
  sample_info info_ TRAA_GUARDED_BY(mutex_);
};

class traa_histogram_map {
public:
  traa_histogram_map() {}
  ~traa_histogram_map() {}

  traa_histogram_map(const traa_histogram_map &) = delete;
  traa_histogram_map &operator=(const traa_histogram_map &) = delete;

  histogram *get_counts_histogram(std::string_view name, int min, int max, int bucket_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    if (it != map_.end())
      return reinterpret_cast<histogram *>(it->second.get());

    traa_histogram *hist = new traa_histogram(name, min, max, bucket_count);
    map_.emplace(name, hist);
    return reinterpret_cast<histogram *>(hist);
  }

  histogram *get_enumeration_histogram(std::string_view name, int boundary) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    if (it != map_.end())
      return reinterpret_cast<histogram *>(it->second.get());

    traa_histogram *hist = new traa_histogram(name, 1, boundary, boundary + 1);
    map_.emplace(name, hist);
    return reinterpret_cast<histogram *>(hist);
  }

  void
  get_and_reset(std::map<std::string, std::unique_ptr<sample_info>, string_view_cmp> *histograms) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : map_) {
      std::unique_ptr<sample_info> info = kv.second->get_and_reset();
      if (info)
        histograms->insert(std::make_pair(kv.first, std::move(info)));
    }
  }

  // Functions only for testing.
  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : map_)
      kv.second->reset();
  }

  int num_events(std::string_view name, int sample) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    return (it == map_.end()) ? 0 : it->second->num_events(sample);
  }

  int num_samples(std::string_view name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    return (it == map_.end()) ? 0 : it->second->num_samples();
  }

  int min_sample(std::string_view name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    return (it == map_.end()) ? -1 : it->second->min_sample();
  }

  std::map<int, int> samples(std::string_view name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto &it = map_.find(name);
    return (it == map_.end()) ? std::map<int, int>() : it->second->samples();
  }

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::unique_ptr<traa_histogram>, string_view_cmp>
      map_ TRAA_GUARDED_BY(mutex_);
};

// traa_histogram_map is allocated upon call to enable().
// The histogram getter functions, which return pointer values to the histograms
// in the map, are cached in WebRTC. Therefore, this memory is not freed by the
// application (the memory will be reclaimed by the OS).
static std::atomic<traa_histogram_map *> g_traa_histogram_map(nullptr);

void create_map() {
  traa_histogram_map *map = g_traa_histogram_map.load(std::memory_order_acquire);
  if (map == nullptr) {
    traa_histogram_map *new_map = new traa_histogram_map();
    if (!g_traa_histogram_map.compare_exchange_strong(map, new_map))
      delete new_map;
  }
}

// Set the first time we start using histograms. Used to make sure enable() is
// not called thereafter.
#if TRAA_DCHECK_IS_ON
static std::atomic<int> g_traa_histogram_called(0);
#endif

// Gets the map (or nullptr).
traa_histogram_map *get_map() {
#if TRAA_DCHECK_IS_ON
  g_traa_histogram_called.store(1, std::memory_order_release);
#endif
  return g_traa_histogram_map.load();
}
} // namespace

#ifndef TRAA_EXCLUDE_METRICS_DEFAULT
// Implementation of histogram methods in
// webrtc/system_wrappers/interface/metrics.h.

// histogram with exponentially spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
histogram *histogram_factory_get_counts(std::string_view name, int min, int max, int bucket_count) {
  // TODO(asapersson): Alternative implementation will be needed if this
  // histogram type should be truly exponential.
  return histogram_factory_get_counts_linear(name, min, max, bucket_count);
}

// histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
histogram *histogram_factory_get_counts_linear(std::string_view name, int min, int max,
                                               int bucket_count) {
  traa_histogram_map *map = get_map();
  if (!map)
    return nullptr;

  return map->get_counts_histogram(name, min, max, bucket_count);
}

// histogram with linearly spaced buckets.
// Creates (or finds) histogram.
// The returned histogram pointer is cached (and used for adding samples in
// subsequent calls).
histogram *histogram_factory_get_enumeration(std::string_view name, int boundary) {
  traa_histogram_map *map = get_map();
  if (!map)
    return nullptr;

  return map->get_enumeration_histogram(name, boundary);
}

// Our default implementation reuses the non-sparse histogram.
histogram *sparse_histogram_factory_get_enumeration(std::string_view name, int boundary) {
  return histogram_factory_get_enumeration(name, boundary);
}

// Fast path. Adds `sample` to cached `histogram_pointer`.
void histogram_add(histogram *histogram_pointer, int sample) {
  traa_histogram *ptr = reinterpret_cast<traa_histogram *>(histogram_pointer);
  ptr->add(sample);
}

#endif // TRAA_EXCLUDE_METRICS_DEFAULT

sample_info::sample_info(std::string_view name, int min, int max, size_t bucket_count)
    : name(name), min(min), max(max), bucket_count(bucket_count) {}

sample_info::~sample_info() {}

// Implementation of global functions in metrics.h.
void enable() {
  TRAA_DCHECK(g_traa_histogram_map.load() == nullptr);
#if TRAA_DCHECK_IS_ON
  TRAA_DCHECK_EQ(0, g_traa_histogram_called.load(std::memory_order_acquire));
#endif
  create_map();
}

void get_and_reset(
    std::map<std::string, std::unique_ptr<sample_info>, string_view_cmp> *histograms) {
  histograms->clear();
  traa_histogram_map *map = get_map();
  if (map)
    map->get_and_reset(histograms);
}

void reset() {
  traa_histogram_map *map = get_map();
  if (map)
    map->reset();
}

int num_events(std::string_view name, int sample) {
  traa_histogram_map *map = get_map();
  return map ? map->num_events(name, sample) : 0;
}

int num_samples(std::string_view name) {
  traa_histogram_map *map = get_map();
  return map ? map->num_samples(name) : 0;
}

int min_sample(std::string_view name) {
  traa_histogram_map *map = get_map();
  return map ? map->min_sample(name) : -1;
}

std::map<int, int> samples(std::string_view name) {
  traa_histogram_map *map = get_map();
  return map ? map->samples(name) : std::map<int, int>();
}

} // namespace metrics

} // namespace base
} // namespace traa
