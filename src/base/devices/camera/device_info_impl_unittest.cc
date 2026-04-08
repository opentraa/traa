/*
 * Feature: windows-camera-capture
 * Property 4: Capability mapping access consistency
 * Property 5: Best match algorithm correctness
 *
 * **Validates: Requirements 2.2, 2.3, 2.4, 2.5, 2.6, 2.7**
 */

#include "base/devices/camera/device_info_impl.h"
#include "base/devices/camera/video_capture_defines.h"

#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <cstring>

namespace traa {
namespace base {
namespace {

constexpr int kIterations = 200;

// All video types for random generation.
constexpr video_type kAllTypes[] = {
    video_type::k_unknown, video_type::k_i420,  video_type::k_iyuv,
    video_type::k_rgb24,   video_type::k_yuy2,  video_type::k_yv12,
    video_type::k_rgb565,  video_type::k_nv12,  video_type::k_uyvy,
    video_type::k_mjpeg,
};

constexpr int kAllTypesCount = sizeof(kAllTypes) / sizeof(kAllTypes[0]);

// Preferred types for best-match video type preference check.
constexpr video_type kPreferredTypes[] = {
    video_type::k_i420, video_type::k_yuy2,
    video_type::k_yv12, video_type::k_nv12,
};

bool is_preferred_type(video_type t) {
  for (auto pt : kPreferredTypes) {
    if (t == pt)
      return true;
  }
  return false;
}

// Concrete test subclass of device_info_impl.
// Stores a pre-set list of capabilities and populates capture_capabilities_
// from it when create_capability_map is called.
class test_device_info : public device_info_impl {
public:
  test_device_info() = default;
  ~test_device_info() override = default;

  void set_capabilities(const std::vector<video_capture_capability> &caps) {
    preset_capabilities_ = caps;
  }

  uint32_t number_of_devices() override { return 1; }

  int32_t get_device_name(uint32_t /*device_number*/, char * /*device_name_utf8*/,
                          uint32_t /*device_name_length*/, char * /*device_unique_id_utf8*/,
                          uint32_t /*device_unique_id_utf8_length*/,
                          char * /*product_unique_id_utf8*/,
                          uint32_t /*product_unique_id_utf8_length*/) override {
    return 0;
  }

protected:
  int32_t init() override { return 0; }

  int32_t create_capability_map(const char *device_unique_id_utf8) override {
    capture_capabilities_ = preset_capabilities_;

    // Update last_used_device_name_ so subsequent calls with the same ID
    // skip re-creation (matching the real implementation's caching behavior).
    free(last_used_device_name_);
    last_used_device_name_ = nullptr;
    last_used_device_name_length_ = 0;

    if (device_unique_id_utf8) {
      size_t len = strlen(device_unique_id_utf8);
      last_used_device_name_ = static_cast<char *>(malloc(len + 1));
      if (last_used_device_name_) {
        memcpy(last_used_device_name_, device_unique_id_utf8, len + 1);
        last_used_device_name_length_ = static_cast<uint32_t>(len);
      }
    }

    return static_cast<int32_t>(capture_capabilities_.size());
  }

private:
  std::vector<video_capture_capability> preset_capabilities_;
};

// Helper: generate a random video_capture_capability.
video_capture_capability random_capability(std::mt19937 &rng) {
  std::uniform_int_distribution<int32_t> dim_dist(1, 4096);
  std::uniform_int_distribution<int32_t> fps_dist(1, 120);
  std::uniform_int_distribution<int> type_dist(0, kAllTypesCount - 1);
  std::uniform_int_distribution<int> bool_dist(0, 1);

  video_capture_capability cap;
  cap.width = dim_dist(rng);
  cap.height = dim_dist(rng);
  cap.max_fps = fps_dist(rng);
  cap.video_type = kAllTypes[type_dist(rng)];
  cap.interlaced = bool_dist(rng) != 0;
  return cap;
}

// Generate a random list of capabilities with size in [min_size, max_size].
std::vector<video_capture_capability> random_capability_list(std::mt19937 &rng, int min_size,
                                                             int max_size) {
  std::uniform_int_distribution<int> size_dist(min_size, max_size);
  int count = size_dist(rng);
  std::vector<video_capture_capability> caps;
  caps.reserve(count);
  for (int i = 0; i < count; ++i) {
    caps.push_back(random_capability(rng));
  }
  return caps;
}

constexpr const char *kTestDeviceId = "test-device-001";

// ---------------------------------------------------------------------------
// Property 4: Capability mapping access consistency
// **Validates: Requirements 2.2, 2.3, 2.4**
//
// For any device capability list:
// - number_of_capabilities returns the list size
// - For valid indices (0 to size-1), get_capability returns 0 and the correct capability
// - For invalid indices (>= size), get_capability returns -1
// ---------------------------------------------------------------------------

TEST(DeviceInfoImplTest, NumberOfCapabilitiesReturnsListSize) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    auto caps = random_capability_list(rng, 1, 20);
    test_device_info info;
    info.set_capabilities(caps);

    int32_t count = info.number_of_capabilities(kTestDeviceId);
    EXPECT_EQ(count, static_cast<int32_t>(caps.size()))
        << "number_of_capabilities should return list size. iteration=" << i;
  }
}

TEST(DeviceInfoImplTest, GetCapabilityValidIndexReturnsCorrectCapability) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    auto caps = random_capability_list(rng, 1, 20);
    test_device_info info;
    info.set_capabilities(caps);

    // Trigger capability map creation.
    info.number_of_capabilities(kTestDeviceId);

    // Check every valid index.
    for (uint32_t idx = 0; idx < caps.size(); ++idx) {
      video_capture_capability result;
      int32_t ret = info.get_capability(kTestDeviceId, idx, result);
      EXPECT_EQ(ret, 0)
          << "get_capability should return 0 for valid index " << idx
          << ". iteration=" << i;
      EXPECT_EQ(result, caps[idx])
          << "get_capability should return the correct capability at index " << idx
          << ". iteration=" << i;
    }
  }
}

TEST(DeviceInfoImplTest, GetCapabilityInvalidIndexReturnsError) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<uint32_t> offset_dist(0, 100);

  for (int i = 0; i < kIterations; ++i) {
    auto caps = random_capability_list(rng, 1, 20);
    test_device_info info;
    info.set_capabilities(caps);

    // Trigger capability map creation.
    info.number_of_capabilities(kTestDeviceId);

    // Invalid index: >= size.
    uint32_t invalid_idx = static_cast<uint32_t>(caps.size()) + offset_dist(rng);
    video_capture_capability result;
    int32_t ret = info.get_capability(kTestDeviceId, invalid_idx, result);
    EXPECT_EQ(ret, -1)
        << "get_capability should return -1 for invalid index " << invalid_idx
        << " (size=" << caps.size() << "). iteration=" << i;
  }
}

TEST(DeviceInfoImplTest, NumberOfCapabilitiesNullReturnsError) {
  test_device_info info;
  EXPECT_EQ(info.number_of_capabilities(nullptr), -1);
}

// ---------------------------------------------------------------------------
// Property 5: Best match algorithm correctness
// **Validates: Requirements 2.5, 2.6, 2.7**
//
// For any non-empty capability list and any requested capability:
// (a) The result is a capability from the list
// (b) If any capability has height >= requested, result height >= requested
// (c) When sizes and frame rate match, I420/YUY2/YV12/NV12 types are preferred
// ---------------------------------------------------------------------------

TEST(DeviceInfoImplTest, BestMatchResultIsInList) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    auto caps = random_capability_list(rng, 1, 10);
    test_device_info info;
    info.set_capabilities(caps);

    // Trigger capability map creation.
    info.number_of_capabilities(kTestDeviceId);

    video_capture_capability requested = random_capability(rng);
    video_capture_capability result;
    int32_t idx = info.get_best_matched_capability(kTestDeviceId, requested, result);

    ASSERT_GE(idx, 0)
        << "get_best_matched_capability should find a match for non-empty list. iteration=" << i;
    ASSERT_LT(idx, static_cast<int32_t>(caps.size()))
        << "Returned index should be within list bounds. iteration=" << i;

    // The result should equal the capability at the returned index.
    EXPECT_EQ(result, caps[idx])
        << "Result should match the capability at the returned index. iteration=" << i;
  }
}

TEST(DeviceInfoImplTest, BestMatchRespectsHeightPreference) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    auto caps = random_capability_list(rng, 1, 10);
    test_device_info info;
    info.set_capabilities(caps);

    info.number_of_capabilities(kTestDeviceId);

    video_capture_capability requested = random_capability(rng);
    video_capture_capability result;
    int32_t idx = info.get_best_matched_capability(kTestDeviceId, requested, result);
    ASSERT_GE(idx, 0);

    // Check property (b): if any capability has height >= requested,
    // the result should also have height >= requested.
    bool any_height_ge = false;
    for (const auto &cap : caps) {
      if (cap.height >= requested.height) {
        any_height_ge = true;
        break;
      }
    }

    if (any_height_ge) {
      EXPECT_GE(result.height, requested.height)
          << "When capabilities with sufficient height exist, result height should be >= requested."
          << " requested.height=" << requested.height
          << " result.height=" << result.height
          << " iteration=" << i;
    }
  }
}

TEST(DeviceInfoImplTest, BestMatchPrefersPreferredVideoTypes) {
  std::mt19937 rng(std::random_device{}());

  // Construct a scenario where sizes and frame rate match exactly,
  // and verify that preferred types (I420/YUY2/YV12/NV12) are chosen
  // over non-preferred types.
  for (int i = 0; i < kIterations; ++i) {
    // Create a fixed requested capability.
    video_capture_capability requested;
    std::uniform_int_distribution<int32_t> dim_dist(16, 1920);
    std::uniform_int_distribution<int32_t> fps_dist(15, 60);
    requested.width = dim_dist(rng) & ~1;
    requested.height = dim_dist(rng) & ~1;
    requested.max_fps = fps_dist(rng);
    // Request a non-preferred type so the preference logic kicks in.
    requested.video_type = video_type::k_rgb24;

    // Build a capability list with exact size/fps match but different video types.
    // Include at least one preferred type and one non-preferred type.
    std::vector<video_capture_capability> caps;

    // Non-preferred capability (exact match on size and fps).
    video_capture_capability non_preferred;
    non_preferred.width = requested.width;
    non_preferred.height = requested.height;
    non_preferred.max_fps = requested.max_fps;
    non_preferred.video_type = video_type::k_rgb565;
    caps.push_back(non_preferred);

    // Preferred capability (exact match on size and fps).
    video_capture_capability preferred;
    preferred.width = requested.width;
    preferred.height = requested.height;
    preferred.max_fps = requested.max_fps;
    // Pick a random preferred type.
    std::uniform_int_distribution<int> pref_dist(0, 3);
    preferred.video_type = kPreferredTypes[pref_dist(rng)];
    caps.push_back(preferred);

    test_device_info info;
    info.set_capabilities(caps);
    info.number_of_capabilities(kTestDeviceId);

    video_capture_capability result;
    int32_t idx = info.get_best_matched_capability(kTestDeviceId, requested, result);
    ASSERT_GE(idx, 0);

    // When exact size and fps match, a preferred type should be selected.
    EXPECT_TRUE(is_preferred_type(result.video_type) ||
                result.video_type == requested.video_type)
        << "When sizes and fps match, preferred types (I420/YUY2/YV12/NV12) or the "
        << "requested type should be chosen. result.video_type="
        << static_cast<int>(result.video_type)
        << " iteration=" << i;
  }
}

TEST(DeviceInfoImplTest, BestMatchNullDeviceIdReturnsError) {
  test_device_info info;
  std::vector<video_capture_capability> caps = {{640, 480, 30, video_type::k_i420, false}};
  info.set_capabilities(caps);
  info.number_of_capabilities(kTestDeviceId);

  video_capture_capability requested;
  requested.width = 640;
  requested.height = 480;
  requested.max_fps = 30;
  video_capture_capability result;
  EXPECT_EQ(info.get_best_matched_capability(nullptr, requested, result), -1);
}

} // namespace
} // namespace base
} // namespace traa
