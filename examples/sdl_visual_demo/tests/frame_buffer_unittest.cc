#include "utils/frame_buffer.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: create a dummy I420 frame backed by the given storage vector
// ---------------------------------------------------------------------------

static traa_video_frame make_dummy_frame(std::vector<uint8_t> &storage, int w,
                                         int h) {
  size_t size = static_cast<size_t>(w) * h * 3 / 2;
  storage.resize(size, 0x80);
  traa_video_frame frame{};
  frame.data = storage.data();
  frame.data_length = static_cast<int32_t>(size);
  frame.width = w;
  frame.height = h;
  frame.format = TRAA_VIDEO_FRAME_FORMAT_I420;
  return frame;
}

// ===========================================================================
// Feature: sdl-visual-demo, Property 7: 线程安全帧缓冲区正确性
// Validates: Requirements 13.1, 13.2, 13.3
//
// For any concurrent push/pop operations, frame_buffer guarantees:
// (1) no data races, (2) pop returns the latest pushed frame,
// (3) consecutive pops without push return false on the second.
// ===========================================================================

// ---------------------------------------------------------------------------
// Property test (100 iterations): Push random frames with random even
// dimensions (2-100), verify has_new_frame() returns true and
// buffer_width/height match the last pushed frame.
// ---------------------------------------------------------------------------

TEST(FrameBufferProperty, PushRandomFramesUpdatesState) {
  std::mt19937 rng(42);
  // Even numbers only for I420 (width and height must be even)
  std::uniform_int_distribution<int> dim_dist(1, 50);

  for (int iter = 0; iter < 100; ++iter) {
    frame_buffer fb;

    // Push 1-5 random frames, keep track of the last one
    int num_pushes = std::uniform_int_distribution<int>(1, 5)(rng);
    int last_w = 0;
    int last_h = 0;

    for (int p = 0; p < num_pushes; ++p) {
      int w = dim_dist(rng) * 2; // even number in [2, 100]
      int h = dim_dist(rng) * 2;
      last_w = w;
      last_h = h;

      std::vector<uint8_t> storage;
      traa_video_frame frame = make_dummy_frame(storage, w, h);
      fb.push(&frame);
    }

    EXPECT_TRUE(fb.has_new_frame())
        << "iter=" << iter << " has_new_frame should be true after push";
    EXPECT_EQ(fb.buffer_width(), last_w)
        << "iter=" << iter << " width mismatch";
    EXPECT_EQ(fb.buffer_height(), last_h)
        << "iter=" << iter << " height mismatch";
  }
}

// ===========================================================================
// Unit tests
// Validates: Requirements 13.1, 13.2, 13.3
// ===========================================================================

// ---------------------------------------------------------------------------
// PopWithoutPushReturnsFalse — empty buffer, pop_to_texture returns false
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, PopWithoutPushReturnsFalse) {
  frame_buffer fb;
  SDL_Texture *texture = nullptr;
  int w = 0, h = 0;
  bool result = fb.pop_to_texture(nullptr, &texture, &w, &h);
  EXPECT_FALSE(result);
  EXPECT_EQ(texture, nullptr);
}

// ---------------------------------------------------------------------------
// PushThenHasNewFrame — push a valid frame, has_new_frame() returns true
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, PushThenHasNewFrame) {
  frame_buffer fb;
  EXPECT_FALSE(fb.has_new_frame());

  std::vector<uint8_t> storage;
  traa_video_frame frame = make_dummy_frame(storage, 4, 4);
  fb.push(&frame);

  EXPECT_TRUE(fb.has_new_frame());
}

// ---------------------------------------------------------------------------
// ConsecutivePushKeepsLatest — push A (4x4), push B (8x8), verify latest dims
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, ConsecutivePushKeepsLatest) {
  frame_buffer fb;

  std::vector<uint8_t> storage_a;
  traa_video_frame frame_a = make_dummy_frame(storage_a, 4, 4);
  fb.push(&frame_a);

  std::vector<uint8_t> storage_b;
  traa_video_frame frame_b = make_dummy_frame(storage_b, 8, 8);
  fb.push(&frame_b);

  EXPECT_EQ(fb.buffer_width(), 8);
  EXPECT_EQ(fb.buffer_height(), 8);
  EXPECT_TRUE(fb.has_new_frame());
}

// ---------------------------------------------------------------------------
// PushInvalidFrameIgnored — push nullptr frame, has_new_frame() stays false
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, PushInvalidFrameIgnored) {
  frame_buffer fb;
  fb.push(nullptr);
  EXPECT_FALSE(fb.has_new_frame());
}

// ---------------------------------------------------------------------------
// PushFrameWithNullDataIgnored — push frame with nullptr data, ignored
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, PushFrameWithNullDataIgnored) {
  frame_buffer fb;
  traa_video_frame frame{};
  frame.data = nullptr;
  frame.width = 4;
  frame.height = 4;
  fb.push(&frame);
  EXPECT_FALSE(fb.has_new_frame());
}


// ---------------------------------------------------------------------------
// Concurrent test: N threads push frames simultaneously, verify no crash
// and has_new_frame() is true after all threads complete.
// ---------------------------------------------------------------------------

TEST(FrameBufferUnit, ConcurrentPushNoDataRace) {
  frame_buffer fb;
  constexpr int num_threads = 8;
  constexpr int pushes_per_thread = 50;

  std::vector<std::thread> threads;
  threads.reserve(num_threads);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&fb, t, pushes_per_thread]() {
      for (int i = 0; i < pushes_per_thread; ++i) {
        int w = ((t + 1) * 2);  // even width per thread
        int h = ((i + 1) * 2);  // even height per iteration
        std::vector<uint8_t> storage;
        traa_video_frame frame = make_dummy_frame(storage, w, h);
        fb.push(&frame);
      }
    });
  }

  for (auto &th : threads) {
    th.join();
  }

  // After all threads complete, at least one push succeeded
  EXPECT_TRUE(fb.has_new_frame());
  EXPECT_GT(fb.buffer_width(), 0);
  EXPECT_GT(fb.buffer_height(), 0);
}
