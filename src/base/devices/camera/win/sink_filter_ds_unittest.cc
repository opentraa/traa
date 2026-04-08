/*
 *  Copyright (c) 2024 The traa authors. All Rights Reserved.
 *
 *  Property-based tests for DirectShow Sink Filter.
 */

#include "base/devices/camera/win/sink_filter_ds.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <objbase.h>
#include <random>
#include <vector>
#include <windows.h>

namespace traa {
namespace base {
namespace {

constexpr int kIterations = 200;

// The five expected format types in default priority order.
static const video_type kDefaultOrder[] = {video_type::k_i420, video_type::k_yuy2,
                                           video_type::k_rgb24, video_type::k_uyvy,
                                           video_type::k_mjpeg};
static constexpr int kNumFormats = 5;

// Map a MEDIASUBTYPE GUID back to our video_type enum.
video_type subtype_to_video_type(const GUID &subtype) {
  if (subtype == MEDIASUBTYPE_I420)
    return video_type::k_i420;
  if (subtype == MEDIASUBTYPE_YUY2)
    return video_type::k_yuy2;
  if (subtype == MEDIASUBTYPE_RGB24)
    return video_type::k_rgb24;
  if (subtype == MEDIASUBTYPE_UYVY)
    return video_type::k_uyvy;
  if (subtype == MEDIASUBTYPE_MJPG)
    return video_type::k_mjpeg;
  return video_type::k_unknown;
}

// Test fixture that initializes COM for all tests in this file.
class SinkFilterDSTest : public ::testing::Test {
protected:
  void SetUp() override { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
  void TearDown() override { CoUninitialize(); }
};

// ---------------------------------------------------------------------------
// Property 8: Media type enumerator priority
// Feature: windows-camera-capture, Property 8: media type enumerator priority
// **Validates: Requirements 5.9, 5.10**
// ---------------------------------------------------------------------------

TEST_F(SinkFilterDSTest, MediaTypeEnumeratorPriority) {
  std::mt19937 rng(std::random_device{}());
  // Random dimensions and fps for the capability.
  std::uniform_int_distribution<int32_t> dim_dist(16, 1920);
  std::uniform_int_distribution<int32_t> fps_dist(1, 60);
  // Pick a random video_type from the supported set (or an unsupported one).
  std::uniform_int_distribution<int> type_picker(0, kNumFormats); // 0..5, 5 = unsupported

  for (int iter = 0; iter < kIterations; ++iter) {
    video_capture_capability cap;
    cap.width = dim_dist(rng);
    cap.height = dim_dist(rng);
    cap.max_fps = fps_dist(rng);

    int type_idx = type_picker(rng);
    if (type_idx < kNumFormats) {
      cap.video_type = kDefaultOrder[type_idx];
    } else {
      // Use an unsupported type (e.g. k_nv12) to test default order.
      cap.video_type = video_type::k_nv12;
    }

    // Create a capture_sink_filter + capture_input_pin to get EnumMediaTypes.
    // We pass nullptr for capture_observer since we won't call process_captured_frame.
    auto *sink_filter = new com_ref_count<capture_sink_filter>(nullptr);
    // Set the requested capability on the filter (which forwards to the pin).
    sink_filter->set_requested_capability(cap);

    // Get the input pin via EnumPins.
    IEnumPins *enum_pins = nullptr;
    ASSERT_EQ(sink_filter->EnumPins(&enum_pins), S_OK);
    IPin *pin = nullptr;
    ULONG fetched = 0;
    ASSERT_EQ(enum_pins->Next(1, &pin, &fetched), S_OK);
    ASSERT_EQ(fetched, 1u);
    enum_pins->Release();

    // Get the media type enumerator from the pin.
    IEnumMediaTypes *enum_types = nullptr;
    ASSERT_EQ(pin->EnumMediaTypes(&enum_types), S_OK);

    // Collect all enumerated types.
    std::vector<video_type> enumerated_types;
    AM_MEDIA_TYPE *mt = nullptr;
    while (true) {
      ULONG mt_fetched = 0;
      enum_types->Next(1, &mt, &mt_fetched);
      if (mt_fetched == 0)
        break;
      video_type vt = subtype_to_video_type(mt->subtype);
      enumerated_types.push_back(vt);
      // Free the media type allocated by the enumerator.
      free_media_type(mt);
    }
    enum_types->Release();
    pin->Release();

    // Property: The enumerator should return exactly 5 types.
    ASSERT_EQ(enumerated_types.size(), static_cast<size_t>(kNumFormats))
        << "Expected 5 media types at iteration " << iter;

    // Property: All five expected types should be present.
    for (int i = 0; i < kNumFormats; ++i) {
      EXPECT_NE(std::find(enumerated_types.begin(), enumerated_types.end(), kDefaultOrder[i]),
                enumerated_types.end())
          << "Missing format " << static_cast<int>(kDefaultOrder[i]) << " at iteration " << iter;
    }

    // Property: When the requested video_type is in the supported list,
    // it should appear first in the enumeration.
    bool requested_is_supported =
        std::find(std::begin(kDefaultOrder), std::end(kDefaultOrder), cap.video_type) !=
        std::end(kDefaultOrder);
    if (requested_is_supported) {
      EXPECT_EQ(enumerated_types[0], cap.video_type)
          << "Requested type " << static_cast<int>(cap.video_type)
          << " should be first at iteration " << iter;
    } else {
      // When unsupported, default order should be preserved.
      for (int i = 0; i < kNumFormats; ++i) {
        EXPECT_EQ(enumerated_types[i], kDefaultOrder[i])
            << "Default order mismatch at position " << i << " iteration " << iter;
      }
    }

    sink_filter->Release();
  }
}

} // namespace
} // namespace base
} // namespace traa
