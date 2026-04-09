#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif // __APPLE__

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)

#include <gtest/gtest.h>

#include <traa/traa.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <mutex>
#include <random>
#include <thread>

// Redefine the fixture since TEST_F requires the class visible in this translation unit.
class traa_engine_test : public testing::Test {
protected:
  void SetUp() override {
    traa_config config;
    config.userdata = reinterpret_cast<traa_userdata>(0x12345678);
    config.event_handler.on_error = [](const traa_userdata, traa_error, const char *) {};
    EXPECT_EQ(traa_init(&config), TRAA_ERROR_NONE);
  }
  void TearDown() override { traa_release(); }
};

// ---------------------------------------------------------------------------
// Frame receipt tracker used by screen capture tests
// ---------------------------------------------------------------------------
struct screen_frame_receipt {
  std::atomic<int> frame_count{0};
  std::atomic<bool> has_valid_frame{false};
  std::mutex mtx;
};

static void screen_capture_on_video_frame(const traa_userdata userdata,
                                          const traa_video_frame *frame) {
  auto *r = static_cast<screen_frame_receipt *>(userdata);
  r->frame_count.fetch_add(1, std::memory_order_relaxed);
  if (frame && frame->data && frame->data_length > 0 && frame->width > 0 && frame->height > 0) {
    r->has_valid_frame.store(true, std::memory_order_relaxed);
  }
}

// Helper: return the first available screen source id, or TRAA_INVALID_SCREEN_ID if none.
// Returns TRAA_ERROR_PERMISSION_DENIED via out param if permission is denied.
static int64_t get_first_screen_source_id(int *out_ret = nullptr) {
  traa_screen_source_info *infos = nullptr;
  int count = 0;
  traa_size icon_size(0, 0);
  traa_size thumbnail_size(0, 0);
  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size, 0, &infos, &count);
  if (out_ret) {
    *out_ret = ret;
  }
  if (ret != TRAA_ERROR_NONE || count == 0) {
    if (infos) {
      traa_free_screen_source_info(infos, count);
    }
    return TRAA_INVALID_SCREEN_ID;
  }
  int64_t source_id = infos[0].id;
  traa_free_screen_source_info(infos, count);
  return source_id;
}

// ===================================================================
// PARAMETER VALIDATION tests  (Requirements 12.1 - 12.4)
// ===================================================================

// 12.1: nullptr config → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, StartWithNullConfig) {
  EXPECT_EQ(traa_start_screen_capture(nullptr), TRAA_ERROR_INVALID_ARGUMENT);
}

// 12.2: on_video_frame nullptr → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, StartWithNullCallback) {
  traa_screen_capture_config config;
  config.source_id = 1;
  config.on_video_frame = nullptr;
  EXPECT_EQ(traa_start_screen_capture(&config), TRAA_ERROR_INVALID_ARGUMENT);
}

// 12.3: source_id = TRAA_INVALID_SCREEN_ID → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, StartWithInvalidSourceId) {
  traa_screen_capture_config config;
  config.source_id = TRAA_INVALID_SCREEN_ID;
  config.on_video_frame = screen_capture_on_video_frame;
  EXPECT_EQ(traa_start_screen_capture(&config), TRAA_ERROR_INVALID_ARGUMENT);
}

// 12.4: stop with TRAA_INVALID_SCREEN_ID → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, StopWithInvalidSourceId) {
  EXPECT_EQ(traa_stop_screen_capture(TRAA_INVALID_SCREEN_ID), TRAA_ERROR_INVALID_ARGUMENT);
}

// ===================================================================
// LIFECYCLE tests  (Requirements 12.5, 12.9)
// ===================================================================

// 12.5: stop with a valid but non-started source_id → TRAA_ERROR_NOT_FOUND
TEST_F(traa_engine_test, StopNonExistentCapture) {
  // Use an arbitrary source_id that is not TRAA_INVALID_SCREEN_ID
  EXPECT_EQ(traa_stop_screen_capture(99999), TRAA_ERROR_NOT_FOUND);
}

// 12.9: start same source_id twice → TRAA_ERROR_ALREADY_EXISTS
TEST_F(traa_engine_test, DuplicateStart) {
  int enum_ret = 0;
  int64_t source_id = get_first_screen_source_id(&enum_ret);
  if (enum_ret == TRAA_ERROR_PERMISSION_DENIED ||
      enum_ret == TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED) {
    GTEST_SKIP() << "Screen source enumeration permission denied";
  }
  if (source_id == TRAA_INVALID_SCREEN_ID) {
    GTEST_SKIP() << "No screen sources available";
  }

  screen_frame_receipt receipt;
  traa_screen_capture_config config;
  config.source_id = source_id;
  config.on_video_frame = screen_capture_on_video_frame;
  config.userdata = &receipt;

  int start_ret = traa_start_screen_capture(&config);
  if (start_ret == TRAA_ERROR_PERMISSION_DENIED) {
    GTEST_SKIP() << "Screen capture permission denied";
  }
  ASSERT_EQ(start_ret, TRAA_ERROR_NONE);

  EXPECT_EQ(traa_start_screen_capture(&config), TRAA_ERROR_ALREADY_EXISTS);

  // Cleanup
  traa_stop_screen_capture(source_id);
}

// ===================================================================
// INTEGRATION test  (Requirements 12.6, 12.7, 12.8, 12.10)
// ===================================================================

// 12.6-12.8, 12.10: Enumerate → start → receive frames → validate → stop
TEST_F(traa_engine_test, CaptureAndReceiveFrames) {
  int enum_ret = 0;
  int64_t source_id = get_first_screen_source_id(&enum_ret);
  if (enum_ret == TRAA_ERROR_PERMISSION_DENIED ||
      enum_ret == TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED) {
    GTEST_SKIP() << "Screen source enumeration permission denied";
  }
  if (source_id == TRAA_INVALID_SCREEN_ID) {
    GTEST_SKIP() << "No screen sources available";
  }

  screen_frame_receipt receipt;
  traa_screen_capture_config config;
  config.source_id = source_id;
  config.on_video_frame = screen_capture_on_video_frame;
  config.userdata = &receipt;

  // 12.6: start capture with valid config
  int start_ret = traa_start_screen_capture(&config);
  if (start_ret == TRAA_ERROR_PERMISSION_DENIED) {
    GTEST_SKIP() << "Screen capture permission denied";
  }
  ASSERT_EQ(start_ret, TRAA_ERROR_NONE);

  // 12.7: Wait up to 5 seconds for at least one frame
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (receipt.frame_count.load(std::memory_order_relaxed) == 0 &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  EXPECT_GT(receipt.frame_count.load(std::memory_order_relaxed), 0)
      << "No frame received within 5 seconds";

  // Validate frame data: data non-null, data_length > 0, width > 0, height > 0
  EXPECT_TRUE(receipt.has_valid_frame.load(std::memory_order_relaxed))
      << "Received frame(s) but none had valid data";

  // 12.8: Stop capture returns TRAA_ERROR_NONE
  EXPECT_EQ(traa_stop_screen_capture(source_id), TRAA_ERROR_NONE);
}

// ===================================================================
// PROPERTY-BASED TEST  (Property 3: Stop non-existent capture returns NOT_FOUND)
// ===================================================================

// **Validates: Requirements 2.4**
//
// Property 3: For any source_id (not equal to TRAA_INVALID_SCREEN_ID and not in the active
// capture map), calling traa_stop_screen_capture(source_id) should return TRAA_ERROR_NOT_FOUND.
//
// Tag: Feature: screen-capture-preview, Property 3: Stop non-existent capture returns NOT_FOUND
TEST_F(traa_engine_test, Property3_StopNonExistentCaptureReturnsNotFound) {
  // Use a simple LCG-based PRNG seeded from std::random_device for reproducibility logging.
  std::random_device rd;
  const uint64_t seed = rd();
  std::mt19937_64 rng(seed);
  // Log the seed so failures can be reproduced.
  std::cout << "[  SEED    ] Property3 seed = " << seed << std::endl;

  std::uniform_int_distribution<int64_t> dist(std::numeric_limits<int64_t>::min(),
                                              std::numeric_limits<int64_t>::max());

  constexpr int k_iterations = 100;
  for (int i = 0; i < k_iterations; ++i) {
    int64_t source_id = dist(rng);
    // Ensure we never use TRAA_INVALID_SCREEN_ID — regenerate if we hit it.
    while (source_id == TRAA_INVALID_SCREEN_ID) {
      source_id = dist(rng);
    }

    EXPECT_EQ(traa_stop_screen_capture(source_id), TRAA_ERROR_NOT_FOUND)
        << "iteration " << i << ", source_id = " << source_id;
  }
}

#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE && (!defined(TARGET_OS_VISION)
       // || !TARGET_OS_VISION)) || __linux__
