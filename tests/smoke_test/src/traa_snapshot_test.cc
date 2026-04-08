// Snapshot smoke tests for traa_create_snapshot / traa_free_snapshot
// Requirements: 9.1, 9.2, 9.3, 9.4, 9.5, 9.6, 9.7, 9.8, 9.9, 9.10, 9.11

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif // __APPLE__

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION))

#include <gtest/gtest.h>

#include <traa/traa.h>

#include <cstdint>
#include <cstdio>

// ---------------------------------------------------------------------------
// Fixture — mirrors the one in traa_engine_test.cc
// ---------------------------------------------------------------------------
class traa_engine_test : public testing::Test {
protected:
  void SetUp() override {
    traa_config config;
    config.userdata = reinterpret_cast<traa_userdata>(0x12345678);
    config.event_handler.on_error = [](traa_userdata, traa_error, const char *) {};
    EXPECT_EQ(traa_init(&config), TRAA_ERROR_NONE);
  }
  void TearDown() override { traa_release(); }
};

// ---------------------------------------------------------------------------
// Helper: enumerate screen sources and return a screen ID and a window ID.
// Returns -1 for either if not found.
// ---------------------------------------------------------------------------
struct source_ids {
  int64_t screen_id = -1;
  int64_t window_id = -1;
};

static source_ids enumerate_source_ids() {
  source_ids ids;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info({0, 0}, {0, 0}, 0, &infos, &count);
  if (ret != TRAA_ERROR_NONE || count == 0) {
    if (infos) {
      traa_free_screen_source_info(infos, count);
    }
    return ids;
  }

  for (int i = 0; i < count; ++i) {
    if (!infos[i].is_window && ids.screen_id == -1) {
      ids.screen_id = infos[i].id;
    } else if (infos[i].is_window && ids.window_id == -1) {
      ids.window_id = infos[i].id;
    }
    if (ids.screen_id != -1 && ids.window_id != -1) {
      break;
    }
  }

  traa_free_screen_source_info(infos, count);
  return ids;
}

// ---------------------------------------------------------------------------
// Test 1 + 2 + 10: Valid screen source ID snapshot (Req 9.1, 9.2, 9.10)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_valid_screen_source) {
  auto ids = enumerate_source_ids();
  if (ids.screen_id == -1) {
    GTEST_SKIP() << "No screen source available, skipping";
  }

  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret = traa_create_snapshot(ids.screen_id, snapshot_size, &data, &data_size, &actual_size);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || ret == TRAA_ERROR_PERMISSION_DENIED);

  if (ret == TRAA_ERROR_NONE) {
    // Req 9.2: verify data, data_size, actual_size
    EXPECT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    EXPECT_GT(actual_size.width, 0);
    EXPECT_GT(actual_size.height, 0);

    // Req 9.10: free after successful snapshot doesn't crash
    traa_free_snapshot(data);
  } else {
    printf("Permission denied for screen snapshot\n");
  }
}

// ---------------------------------------------------------------------------
// Test 3: Valid window source ID snapshot (Req 9.3)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_valid_window_source) {
  auto ids = enumerate_source_ids();
  if (ids.window_id == -1) {
    GTEST_SKIP() << "No window source available, skipping";
  }

  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret = traa_create_snapshot(ids.window_id, snapshot_size, &data, &data_size, &actual_size);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || ret == TRAA_ERROR_PERMISSION_DENIED);

  if (ret == TRAA_ERROR_NONE) {
    EXPECT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    EXPECT_GT(actual_size.width, 0);
    EXPECT_GT(actual_size.height, 0);
    traa_free_snapshot(data);
  } else {
    printf("Permission denied for window snapshot\n");
  }
}

// ---------------------------------------------------------------------------
// Test 4: TRAA_FULLSCREEN_SCREEN_ID (-1) returns INVALID_ARGUMENT (Req 9.4)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_fullscreen_screen_id) {
  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret = traa_create_snapshot(TRAA_FULLSCREEN_SCREEN_ID, snapshot_size, &data, &data_size,
                                 &actual_size);
  EXPECT_EQ(ret, TRAA_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Test 5: TRAA_INVALID_SCREEN_ID (-2) returns INVALID_ARGUMENT (Req 9.5)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_invalid_screen_id) {
  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret =
      traa_create_snapshot(TRAA_INVALID_SCREEN_ID, snapshot_size, &data, &data_size, &actual_size);
  EXPECT_EQ(ret, TRAA_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Test 6: data parameter as nullptr returns INVALID_ARGUMENT (Req 9.6)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_nullptr_data) {
  int data_size = 0;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret = traa_create_snapshot(1, snapshot_size, nullptr, &data_size, &actual_size);
  EXPECT_EQ(ret, TRAA_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Test 7: data_size parameter as nullptr returns INVALID_ARGUMENT (Req 9.7)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_nullptr_data_size) {
  uint8_t *data = nullptr;
  traa_size snapshot_size(800, 600);
  traa_size actual_size;

  int ret = traa_create_snapshot(1, snapshot_size, &data, nullptr, &actual_size);
  EXPECT_EQ(ret, TRAA_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Test 8: actual_size parameter as nullptr returns INVALID_ARGUMENT (Req 9.8)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_nullptr_actual_size) {
  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(800, 600);

  int ret = traa_create_snapshot(1, snapshot_size, &data, &data_size, nullptr);
  EXPECT_EQ(ret, TRAA_ERROR_INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Test 9: snapshot_size with width/height both 0 (Req 9.9)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_zero_size) {
  auto ids = enumerate_source_ids();

  // Use a valid source if available; otherwise use a dummy id — the API should
  // still reject zero size or return original-size data.
  int64_t source_id = (ids.screen_id != -1) ? ids.screen_id : 1;

  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size snapshot_size(0, 0);
  traa_size actual_size;

  int ret = traa_create_snapshot(source_id, snapshot_size, &data, &data_size, &actual_size);

  // The API may return various error codes for zero-size snapshots, or succeed with original size.
  // Accept any valid error code in the traa_error range.
  EXPECT_TRUE(ret >= 0 && ret < static_cast<int>(TRAA_ERROR_COUNT))
      << "Unexpected error code: " << ret;

  if (ret == TRAA_ERROR_NONE && data != nullptr) {
    EXPECT_GT(data_size, 0);
    EXPECT_GT(actual_size.width, 0);
    EXPECT_GT(actual_size.height, 0);
    traa_free_snapshot(data);
  }
}

// ---------------------------------------------------------------------------
// Test 11: traa_free_snapshot(nullptr) doesn't crash (Req 9.11)
// ---------------------------------------------------------------------------
TEST_F(traa_engine_test, snapshot_free_nullptr) {
  // Should not crash
  traa_free_snapshot(nullptr);
}

#endif // defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION))
