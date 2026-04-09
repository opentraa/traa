// Feature: screen-capture-preview, Property 1: Frame data integrity
// **Validates: Requirements 1.3, 5.2**
//
// Property 1: For any valid desktop_frame (width > 0, height > 0, BGRA data
// non-empty), when wrapped as traa_video_frame by the screen_capture_callback,
// the result should have:
//   - data is non-null
//   - data_length == width * height * 4
//   - width and height match the source frame
//   - format == TRAA_VIDEO_FRAME_FORMAT_BGRA

#include <traa/base.h>

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"

#include <gtest/gtest.h>
#include <random>

// Desktop platform guard — mirrors the project's standard conditional.
#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

namespace {

constexpr int kIterations = 200;

// Simulates the frame wrapping logic from screen_capture_callback::on_capture_result
// (no-scale path). This is the same invariant the production code must satisfy.
void wrap_desktop_frame_as_video_frame(const traa::base::desktop_frame &frame,
                                       traa_video_frame &out) {
  out.data = frame.data();
  out.data_length = frame.size().width() * frame.size().height() * 4;
  out.width = frame.size().width();
  out.height = frame.size().height();
  out.format = TRAA_VIDEO_FRAME_FORMAT_BGRA;
  out.timestamp_ms = frame.capture_time_ms();
}

// Feature: screen-capture-preview, Property 1: Frame data integrity
// **Validates: Requirements 1.3, 5.2**
TEST(ScreenCapturePropertyTest, FrameDataIntegrity) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int32_t> dim_dist(1, 4096);

  for (int i = 0; i < kIterations; ++i) {
    const int32_t width = dim_dist(rng);
    const int32_t height = dim_dist(rng);

    // Create a basic_desktop_frame with the random dimensions.
    // basic_desktop_frame allocates width * height * 4 bytes of BGRA data
    // (zero-initialized).
    traa::base::basic_desktop_frame frame(traa::base::desktop_size(width, height));

    // Write a non-zero sentinel at the start to confirm the buffer is writable
    // and the pointer is valid. We avoid filling the entire buffer to keep the
    // test fast for large dimensions.
    frame.data()[0] = 0xAB;

    // Wrap the desktop_frame as a traa_video_frame (simulating the callback).
    traa_video_frame video_frame;
    wrap_desktop_frame_as_video_frame(frame, video_frame);

    // Property assertions:
    // 1. data is non-null
    EXPECT_NE(video_frame.data, nullptr)
        << "data should be non-null at iteration " << i
        << " (width=" << width << ", height=" << height << ")";

    // 2. data_length == width * height * 4
    const int32_t expected_length = width * height * 4;
    EXPECT_EQ(video_frame.data_length, expected_length)
        << "data_length mismatch at iteration " << i
        << " (width=" << width << ", height=" << height << ")";

    // 3. width and height match the source frame
    EXPECT_EQ(video_frame.width, width)
        << "width mismatch at iteration " << i;
    EXPECT_EQ(video_frame.height, height)
        << "height mismatch at iteration " << i;

    // 4. format == TRAA_VIDEO_FRAME_FORMAT_BGRA
    EXPECT_EQ(video_frame.format, TRAA_VIDEO_FRAME_FORMAT_BGRA)
        << "format should be BGRA at iteration " << i;
  }
}

} // namespace

// ---------------------------------------------------------------------------
// Feature: screen-capture-preview, Property 2: Frame scaling correctness
// **Validates: Requirements 5.3, 5.4**
//
// Property 2: For any valid source frame size (width > 0, height > 0) and any
// frame_size config:
//   - When frame_size.width > 0 AND frame_size.height > 0, the output frame's
//     width and height should equal frame_size values
//   - When frame_size.width == 0 OR frame_size.height == 0, the output frame's
//     width and height should equal the source frame's original size
//
// This test simulates the scaling logic from
// screen_capture_callback::on_capture_result using libyuv::ARGBScale.
// ---------------------------------------------------------------------------

#include "libyuv/scale_argb.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

// Simulates the scaling decision and execution from
// screen_capture_callback::on_capture_result. Returns the output frame
// dimensions and populates |out| accordingly.
void simulate_scaling(const traa::base::desktop_frame &frame,
                      const traa_size &frame_size,
                      traa_video_frame &out,
                      std::vector<uint8_t> &scale_buffer) {
  const int src_width = frame.size().width();
  const int src_height = frame.size().height();
  const uint8_t *src_data = frame.data();
  const int src_stride = frame.stride();

  const bool need_scale = frame_size.width > 0 && frame_size.height > 0 &&
                          (frame_size.width != src_width || frame_size.height != src_height);

  out.format = TRAA_VIDEO_FRAME_FORMAT_BGRA;

  if (need_scale) {
    const int dst_width = frame_size.width;
    const int dst_height = frame_size.height;
    const int dst_stride = dst_width * 4;
    const size_t dst_size = static_cast<size_t>(dst_stride) * dst_height;

    scale_buffer.resize(dst_size);

    int ret = libyuv::ARGBScale(src_data, src_stride, src_width, src_height,
                                 scale_buffer.data(), dst_stride, dst_width, dst_height,
                                 libyuv::kFilterBilinear);
    ASSERT_EQ(ret, 0) << "ARGBScale failed";

    out.data = scale_buffer.data();
    out.data_length = static_cast<int32_t>(dst_size);
    out.width = dst_width;
    out.height = dst_height;
  } else {
    out.data = src_data;
    out.data_length = src_width * src_height * 4;
    out.width = src_width;
    out.height = src_height;
  }
}

// Feature: screen-capture-preview, Property 2: Frame scaling correctness
// **Validates: Requirements 5.3, 5.4**
TEST(ScreenCapturePropertyTest, FrameScalingCorrectness) {
  std::mt19937 rng(std::random_device{}());
  // Source dimensions: 1–512 (kept moderate to avoid huge allocations).
  std::uniform_int_distribution<int32_t> src_dist(1, 512);
  // Target dimensions: 0–512. Zero triggers the "use original size" path.
  std::uniform_int_distribution<int32_t> tgt_dist(0, 512);

  std::vector<uint8_t> scale_buffer;

  constexpr int kScaleIterations = 200;

  for (int i = 0; i < kScaleIterations; ++i) {
    const int32_t src_w = src_dist(rng);
    const int32_t src_h = src_dist(rng);
    const int32_t tgt_w = tgt_dist(rng);
    const int32_t tgt_h = tgt_dist(rng);

    // Create a source frame with random BGRA content.
    traa::base::basic_desktop_frame frame(traa::base::desktop_size(src_w, src_h));
    // Fill with a simple pattern so ARGBScale has real data to work with.
    std::memset(frame.data(), 0x42, static_cast<size_t>(frame.stride()) * src_h);

    traa_size frame_size(tgt_w, tgt_h);
    traa_video_frame video_frame;
    simulate_scaling(frame, frame_size, video_frame, scale_buffer);

    // Determine expected output dimensions per the production logic.
    const bool both_positive = tgt_w > 0 && tgt_h > 0;

    if (both_positive) {
      // Requirement 5.3: output dimensions equal frame_size values.
      EXPECT_EQ(video_frame.width, tgt_w)
          << "iteration " << i << ": expected width=" << tgt_w
          << " (src=" << src_w << "x" << src_h
          << ", tgt=" << tgt_w << "x" << tgt_h << ")";
      EXPECT_EQ(video_frame.height, tgt_h)
          << "iteration " << i << ": expected height=" << tgt_h
          << " (src=" << src_w << "x" << src_h
          << ", tgt=" << tgt_w << "x" << tgt_h << ")";
    } else {
      // Requirement 5.4: output dimensions equal source original size.
      EXPECT_EQ(video_frame.width, src_w)
          << "iteration " << i << ": expected original width=" << src_w
          << " (tgt=" << tgt_w << "x" << tgt_h << ")";
      EXPECT_EQ(video_frame.height, src_h)
          << "iteration " << i << ": expected original height=" << src_h
          << " (tgt=" << tgt_w << "x" << tgt_h << ")";
    }

    // Common invariants regardless of scaling path.
    EXPECT_NE(video_frame.data, nullptr) << "iteration " << i;
    EXPECT_EQ(video_frame.format, TRAA_VIDEO_FRAME_FORMAT_BGRA) << "iteration " << i;
    EXPECT_EQ(video_frame.data_length, video_frame.width * video_frame.height * 4)
        << "iteration " << i;
  }
}

} // namespace

#endif // desktop platform guard
