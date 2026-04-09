// ===========================================================================
// Feature: screen-capture-preview, Property 4: Preview scale bounds
// Validates: Requirement 10.2
//
// For any positive integer preview frame size (preview_w, preview_h) and any
// positive float available area size (avail_w, avail_h), the computed scale
// should satisfy:
//   1. scale <= 1.0
//   2. preview_w * scale <= avail_w
//   3. preview_h * scale <= avail_h
//   4. Aspect ratio preservation: the ratio of scaled dimensions matches the
//      original ratio (within floating point tolerance)
//
// The scaling logic under test (from screen_capture_panel::on_render):
//   float scale_x = avail_w / static_cast<float>(preview_w);
//   float scale_y = avail_h / static_cast<float>(preview_h);
//   float scale = (scale_x < scale_y) ? scale_x : scale_y;
//   if (scale > 1.0f) scale = 1.0f;
// ===========================================================================

#include <gtest/gtest.h>

#include <cmath>
#include <random>

namespace {

// Replicates the scaling logic from screen_capture_panel::on_render
struct scale_result {
  float scale;
  float dw; // preview_w * scale
  float dh; // preview_h * scale
};

scale_result compute_preview_scale(int preview_w, int preview_h, float avail_w,
                                   float avail_h) {
  float scale_x = avail_w / static_cast<float>(preview_w);
  float scale_y = avail_h / static_cast<float>(preview_h);
  float scale = (scale_x < scale_y) ? scale_x : scale_y;
  if (scale > 1.0f) {
    scale = 1.0f;
  }

  float dw = static_cast<float>(preview_w) * scale;
  float dh = static_cast<float>(preview_h) * scale;
  return {scale, dw, dh};
}

} // namespace

TEST(PreviewScaleProperty, ScaleBoundsAndAspectRatio) {
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dim_dist(1, 8192);
  std::uniform_real_distribution<float> avail_dist(1.0f, 4096.0f);

  for (int iter = 0; iter < 200; ++iter) {
    int preview_w = dim_dist(rng);
    int preview_h = dim_dist(rng);
    float avail_w = avail_dist(rng);
    float avail_h = avail_dist(rng);

    auto result = compute_preview_scale(preview_w, preview_h, avail_w, avail_h);

    // Property 1: scale <= 1.0
    EXPECT_LE(result.scale, 1.0f)
        << "iter=" << iter << " preview=" << preview_w << "x" << preview_h
        << " avail=" << avail_w << "x" << avail_h;

    // Property 2: preview_w * scale <= avail_w
    EXPECT_LE(result.dw, avail_w + 1e-3f)
        << "iter=" << iter << " preview=" << preview_w << "x" << preview_h
        << " avail=" << avail_w << "x" << avail_h
        << " dw=" << result.dw;

    // Property 3: preview_h * scale <= avail_h
    EXPECT_LE(result.dh, avail_h + 1e-3f)
        << "iter=" << iter << " preview=" << preview_w << "x" << preview_h
        << " avail=" << avail_w << "x" << avail_h
        << " dh=" << result.dh;

    // Property 4: Aspect ratio preservation
    // (preview_w * scale) / (preview_h * scale) == preview_w / preview_h
    // Equivalently: dw * preview_h == dh * preview_w (within tolerance)
    float original_ratio =
        static_cast<float>(preview_w) / static_cast<float>(preview_h);
    float scaled_ratio = result.dw / result.dh;

    // Use relative tolerance for floating point comparison
    float ratio_diff = std::fabs(scaled_ratio - original_ratio);
    float max_ratio = std::fmax(std::fabs(original_ratio), std::fabs(scaled_ratio));
    float relative_err = (max_ratio > 0.0f) ? (ratio_diff / max_ratio) : ratio_diff;

    EXPECT_LT(relative_err, 1e-5f)
        << "iter=" << iter << " preview=" << preview_w << "x" << preview_h
        << " avail=" << avail_w << "x" << avail_h
        << " original_ratio=" << original_ratio
        << " scaled_ratio=" << scaled_ratio;
  }
}
