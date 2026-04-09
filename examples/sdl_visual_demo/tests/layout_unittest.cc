#include "ui/ui_renderer.h"

#include "ui/theme.h"

#include <gtest/gtest.h>

#include <random>

// ---------------------------------------------------------------------------
// helper — check if two rects overlap (strict interior overlap)
// ---------------------------------------------------------------------------

static bool rects_overlap(const SDL_FRect &a, const SDL_FRect &b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
         a.y + a.h > b.y;
}

// ===========================================================================
// Feature: sdl-visual-demo, Property 1: 窗口布局计算不变量
// Validates: Requirements 3.2, 3.5
//
// For any valid window size (width >= 201, height >= 33), the three layout
// rects (sidebar, preview, status_bar) satisfy:
//   1. Sidebar width is always 200 (theme::sidebar_width)
//   2. Status bar height is always 32 (theme::status_bar_height)
//   3. Preview area fills remaining space
//   4. Three areas don't overlap
//   5. Three areas completely cover the window
// ===========================================================================

TEST(LayoutProperty, WindowLayoutCalculationInvariant) {
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> w_dist(201, 4000);
  std::uniform_int_distribution<int> h_dist(33, 3000);

  for (int iter = 0; iter < 100; ++iter) {
    int w = w_dist(rng);
    int h = h_dist(rng);

    SDL_FRect sidebar = ui::calc_sidebar_rect(w, h);
    SDL_FRect preview = ui::calc_preview_rect(w, h);
    SDL_FRect status = ui::calc_status_bar_rect(w, h);

    // 1. Sidebar width is always theme::sidebar_width (200)
    EXPECT_FLOAT_EQ(sidebar.w, static_cast<float>(theme::sidebar_width))
        << "iter=" << iter << " w=" << w << " h=" << h;

    // 2. Status bar height is always theme::status_bar_height (32)
    EXPECT_FLOAT_EQ(status.h, static_cast<float>(theme::status_bar_height))
        << "iter=" << iter << " w=" << w << " h=" << h;

    // 3. Preview area fills remaining space
    EXPECT_FLOAT_EQ(preview.x, static_cast<float>(theme::sidebar_width))
        << "iter=" << iter << " w=" << w << " h=" << h;
    EXPECT_FLOAT_EQ(preview.y, 0.0f)
        << "iter=" << iter << " w=" << w << " h=" << h;
    EXPECT_FLOAT_EQ(preview.w,
                    static_cast<float>(w - theme::sidebar_width))
        << "iter=" << iter << " w=" << w << " h=" << h;
    EXPECT_FLOAT_EQ(preview.h,
                    static_cast<float>(h - theme::status_bar_height))
        << "iter=" << iter << " w=" << w << " h=" << h;

    // 4. Three areas don't overlap
    EXPECT_FALSE(rects_overlap(sidebar, preview))
        << "iter=" << iter << " sidebar and preview overlap";
    EXPECT_FALSE(rects_overlap(sidebar, status))
        << "iter=" << iter << " sidebar and status overlap";
    EXPECT_FALSE(rects_overlap(preview, status))
        << "iter=" << iter << " preview and status overlap";

    // 5. Three areas completely cover the window
    //    Total area of the three rects should equal window area.
    float sidebar_area = sidebar.w * sidebar.h;
    float preview_area = preview.w * preview.h;
    float status_area = status.w * status.h;
    float window_area = static_cast<float>(w) * static_cast<float>(h);

    EXPECT_FLOAT_EQ(sidebar_area + preview_area + status_area, window_area)
        << "iter=" << iter << " w=" << w << " h=" << h
        << " areas don't cover the full window";
  }
}
