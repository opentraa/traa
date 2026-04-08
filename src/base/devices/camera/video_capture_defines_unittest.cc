#include "base/devices/camera/video_capture_defines.h"

#include <gtest/gtest.h>
#include <random>

namespace traa {
namespace base {
namespace {

constexpr int kIterations = 200;

// Valid video types excluding k_unknown.
constexpr video_type kValidTypes[] = {
    video_type::k_i420,  video_type::k_iyuv,  video_type::k_rgb24,
    video_type::k_yuy2,  video_type::k_yv12,  video_type::k_rgb565,
    video_type::k_nv12,  video_type::k_uyvy,  video_type::k_mjpeg,
};

// Helper: compute expected buffer size for a given type and dimensions.
size_t expected_buffer_size(video_type type, int32_t w, int32_t h) {
  switch (type) {
  case video_type::k_i420:
  case video_type::k_iyuv:
  case video_type::k_yv12:
  case video_type::k_nv12: {
    int half_w = (w + 1) / 2;
    int half_h = (h + 1) / 2;
    return static_cast<size_t>(w) * h + static_cast<size_t>(half_w) * half_h * 2;
  }
  case video_type::k_rgb565:
  case video_type::k_yuy2:
  case video_type::k_uyvy:
    return static_cast<size_t>(w) * h * 2;
  case video_type::k_rgb24:
    return static_cast<size_t>(w) * h * 3;
  case video_type::k_mjpeg:
  case video_type::k_unknown:
    return 0;
  }
  return 0;
}

// Feature: windows-camera-capture, Property 9: calc_buffer_size buffer size calculation
// **Validates: Requirements 8.3**
TEST(VideoCaptureDefinesTest, CalcBufferSizeMatchesExpectedFormula) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> type_dist(0,
      static_cast<int>(sizeof(kValidTypes) / sizeof(kValidTypes[0])) - 1);
  std::uniform_int_distribution<int32_t> dim_dist(1, 4096);

  for (int i = 0; i < kIterations; ++i) {
    video_type type = kValidTypes[type_dist(rng)];
    int32_t width = dim_dist(rng);
    int32_t height = dim_dist(rng);

    size_t actual = calc_buffer_size(type, width, height);
    size_t expected = expected_buffer_size(type, width, height);

    EXPECT_EQ(actual, expected)
        << "type=" << static_cast<int>(type)
        << " width=" << width << " height=" << height;
  }
}

TEST(VideoCaptureDefinesTest, CalcBufferSizeUnknownReturnsZero) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int32_t> dim_dist(1, 4096);

  for (int i = 0; i < kIterations; ++i) {
    int32_t width = dim_dist(rng);
    int32_t height = dim_dist(rng);
    EXPECT_EQ(calc_buffer_size(video_type::k_unknown, width, height), 0u);
  }
}

// Helper: generate a random video_capture_capability.
video_capture_capability random_capability(std::mt19937 &rng) {
  std::uniform_int_distribution<int32_t> dim_dist(0, 4096);
  std::uniform_int_distribution<int32_t> fps_dist(0, 120);
  std::uniform_int_distribution<int> type_dist(0, static_cast<int>(video_type::k_mjpeg));
  std::uniform_int_distribution<int> bool_dist(0, 1);

  video_capture_capability cap;
  cap.width = dim_dist(rng);
  cap.height = dim_dist(rng);
  cap.max_fps = fps_dist(rng);
  cap.video_type = static_cast<video_type>(type_dist(rng));
  cap.interlaced = bool_dist(rng) != 0;
  return cap;
}

// Feature: windows-camera-capture, Property 10: video_capture_capability comparison operator consistency
// **Validates: Requirements 8.5, 8.6**
TEST(VideoCaptureDefinesTest, CapabilityEqualityConsistency) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    video_capture_capability a = random_capability(rng);
    video_capture_capability b = random_capability(rng);

    // (a == b) iff !(a != b)
    EXPECT_EQ((a == b), !(a != b))
        << "Consistency violated for random pair at iteration " << i;
  }
}

TEST(VideoCaptureDefinesTest, CapabilityEqualWhenAllFieldsMatch) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    video_capture_capability a = random_capability(rng);
    video_capture_capability b = a; // copy all fields

    EXPECT_TRUE(a == b) << "Equal capabilities should compare equal at iteration " << i;
    EXPECT_FALSE(a != b) << "Equal capabilities should not compare unequal at iteration " << i;
  }
}

TEST(VideoCaptureDefinesTest, CapabilityUnequalWhenAnyFieldDiffers) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> field_dist(0, 4);

  for (int i = 0; i < kIterations; ++i) {
    video_capture_capability a = random_capability(rng);
    video_capture_capability b = a;

    // Mutate exactly one field to make them differ.
    int field = field_dist(rng);
    switch (field) {
    case 0:
      b.width = a.width + 1;
      break;
    case 1:
      b.height = a.height + 1;
      break;
    case 2:
      b.max_fps = a.max_fps + 1;
      break;
    case 3:
      b.video_type = (a.video_type == video_type::k_i420) ? video_type::k_rgb24
                                                          : video_type::k_i420;
      break;
    case 4:
      b.interlaced = !a.interlaced;
      break;
    }

    EXPECT_TRUE(a != b) << "Capabilities differing in field " << field
                        << " should compare unequal at iteration " << i;
    EXPECT_FALSE(a == b) << "Capabilities differing in field " << field
                         << " should not compare equal at iteration " << i;
  }
}

} // namespace
} // namespace base
} // namespace traa
