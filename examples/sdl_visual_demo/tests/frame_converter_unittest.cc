#include "utils/frame_converter.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

// ===========================================================================
// Feature: sdl-visual-demo, Property 5: 帧转换无效输入拒绝
// Validates: Requirements 10.4
//
// For any invalid inputs (nullptr data, width <= 0, height <= 0),
// bgra_to_texture and i420_to_texture return nullptr without crashing.
// ===========================================================================

// ---------------------------------------------------------------------------
// Property test: bgra_to_texture rejects random invalid inputs (100 iters)
// ---------------------------------------------------------------------------

TEST(FrameConverterProperty, BgraToTextureRejectsInvalidInputs) {
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dim_dist(-1000, 0);
  std::uniform_int_distribution<int> choice_dist(0, 3);

  uint8_t dummy_pixel = 0xFF;

  for (int iter = 0; iter < 100; ++iter) {
    // Each iteration picks at least one invalid parameter.
    // choice: 0 = nullptr renderer, 1 = nullptr data,
    //         2 = width <= 0, 3 = height <= 0
    int invalid_choice = choice_dist(rng);

    SDL_Renderer *renderer = nullptr; // always nullptr in unit tests
    const uint8_t *data = &dummy_pixel;
    int width = 64;
    int height = 64;

    switch (invalid_choice) {
    case 0:
      // renderer is already nullptr
      break;
    case 1:
      data = nullptr;
      break;
    case 2:
      width = dim_dist(rng); // <= 0
      break;
    case 3:
      height = dim_dist(rng); // <= 0
      break;
    }

    SDL_Texture *result =
        frame_converter::bgra_to_texture(renderer, data, width, height);
    EXPECT_EQ(result, nullptr)
        << "iter=" << iter << " choice=" << invalid_choice
        << " width=" << width << " height=" << height
        << " data=" << (data ? "valid" : "nullptr")
        << " renderer=" << (renderer ? "valid" : "nullptr");
  }
}

// ---------------------------------------------------------------------------
// Property test: i420_to_texture rejects random invalid inputs (100 iters)
// ---------------------------------------------------------------------------

TEST(FrameConverterProperty, I420ToTextureRejectsInvalidInputs) {
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> dim_dist(-1000, 0);
  std::uniform_int_distribution<int> choice_dist(0, 4);

  uint8_t dummy_data[64] = {};

  for (int iter = 0; iter < 100; ++iter) {
    // choice: 0 = nullptr renderer, 1 = nullptr frame,
    //         2 = frame with nullptr data, 3 = width <= 0, 4 = height <= 0
    int invalid_choice = choice_dist(rng);

    SDL_Renderer *renderer = nullptr; // always nullptr in unit tests
    traa_video_frame frame{};
    frame.data = dummy_data;
    frame.width = 4;
    frame.height = 4;
    const traa_video_frame *frame_ptr = &frame;

    switch (invalid_choice) {
    case 0:
      // renderer is already nullptr
      break;
    case 1:
      frame_ptr = nullptr;
      break;
    case 2:
      frame.data = nullptr;
      break;
    case 3:
      frame.width = dim_dist(rng); // <= 0
      break;
    case 4:
      frame.height = dim_dist(rng); // <= 0
      break;
    }

    SDL_Texture *result =
        frame_converter::i420_to_texture(renderer, frame_ptr);
    EXPECT_EQ(result, nullptr)
        << "iter=" << iter << " choice=" << invalid_choice
        << " frame_ptr=" << (frame_ptr ? "valid" : "nullptr");
  }
}

// ===========================================================================
// Unit tests — specific invalid input edge cases
// Validates: Requirements 10.4
// ===========================================================================

TEST(FrameConverterUnit, BgraToTextureNullptrDataReturnsNullptr) {
  SDL_Texture *result =
      frame_converter::bgra_to_texture(nullptr, nullptr, 100, 100);
  EXPECT_EQ(result, nullptr);
}

TEST(FrameConverterUnit, BgraToTextureZeroWidthReturnsNullptr) {
  uint8_t dummy = 0;
  SDL_Texture *result =
      frame_converter::bgra_to_texture(nullptr, &dummy, 0, 100);
  EXPECT_EQ(result, nullptr);
}

TEST(FrameConverterUnit, BgraToTextureZeroHeightReturnsNullptr) {
  uint8_t dummy = 0;
  SDL_Texture *result =
      frame_converter::bgra_to_texture(nullptr, &dummy, 100, 0);
  EXPECT_EQ(result, nullptr);
}

TEST(FrameConverterUnit, BgraToTextureNullptrRendererReturnsNullptr) {
  uint8_t dummy = 0;
  SDL_Texture *result =
      frame_converter::bgra_to_texture(nullptr, &dummy, 100, 100);
  EXPECT_EQ(result, nullptr);
}

TEST(FrameConverterUnit, I420ToTextureNullptrFrameReturnsNullptr) {
  SDL_Texture *result = frame_converter::i420_to_texture(nullptr, nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST(FrameConverterUnit, I420ToTextureNullptrRendererReturnsNullptr) {
  traa_video_frame frame{};
  uint8_t dummy_data[64] = {};
  frame.data = dummy_data;
  frame.width = 4;
  frame.height = 4;
  SDL_Texture *result = frame_converter::i420_to_texture(nullptr, &frame);
  EXPECT_EQ(result, nullptr);
}
