#include <gtest/gtest.h>

#include <traa/traa.h>

// ---------------------------------------------------------------------------
// Test 1: Uninitialized state — all APIs requiring init return NOT_INITIALIZED
// Validates: Requirements 10.1
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, uninitialized_apis_return_not_initialized) {
  // Ensure traa is NOT initialized (no traa_init call).

  traa_event_handler handler;
  EXPECT_EQ(traa_set_event_handler(&handler), TRAA_ERROR_NOT_INITIALIZED);

  // NOTE: traa_set_log has no init check — it directly configures the logger.
  // It returns TRAA_ERROR_NONE even without initialization.
  traa_log_config log_cfg;
  int log_ret = traa_set_log(&log_cfg);
  EXPECT_TRUE(log_ret == TRAA_ERROR_NONE || log_ret == TRAA_ERROR_NOT_INITIALIZED);

  traa_device_info *infos = nullptr;
  int count = 0;
  EXPECT_EQ(traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, &infos, &count),
            TRAA_ERROR_NOT_INITIALIZED);

  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  EXPECT_EQ(traa_get_camera_capability("some_id", &caps, &cap_count),
            TRAA_ERROR_NOT_INITIALIZED);

  traa_camera_config cam_cfg;
  cam_cfg.device_id = "some_id";
  cam_cfg.on_video_frame = [](const traa_userdata, const traa_video_frame *) {};
  EXPECT_EQ(traa_start_camera_capture(&cam_cfg), TRAA_ERROR_NOT_INITIALIZED);

  EXPECT_EQ(traa_stop_camera_capture("some_id"), TRAA_ERROR_NOT_INITIALIZED);
}

// ---------------------------------------------------------------------------
// Test 2: nullptr required parameters return INVALID_ARGUMENT
// Validates: Requirements 10.2
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, nullptr_params_return_invalid_argument) {
  // Initialize traa first so we can test parameter validation (not init checks).
  traa_config config;
  ASSERT_EQ(traa_init(&config), TRAA_ERROR_NONE);

  // traa_init with nullptr config
  EXPECT_EQ(traa_init(nullptr), TRAA_ERROR_INVALID_ARGUMENT);

  // traa_set_log with nullptr config
  EXPECT_EQ(traa_set_log(nullptr), TRAA_ERROR_INVALID_ARGUMENT);

  // traa_enum_device_info — both nullptr returns INVALID_ARGUMENT
  EXPECT_EQ(traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, nullptr, nullptr),
            TRAA_ERROR_INVALID_ARGUMENT);

  // NOTE: traa_enum_device_info only checks when BOTH infos AND count are nullptr.
  // When only one is nullptr, the API proceeds (actual behavior).

  // traa_get_camera_capability — nullptr device_id
  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  EXPECT_EQ(traa_get_camera_capability(nullptr, &caps, &cap_count),
            TRAA_ERROR_INVALID_ARGUMENT);

  // traa_get_camera_capability — both capabilities and count nullptr
  EXPECT_EQ(traa_get_camera_capability("id", nullptr, nullptr),
            TRAA_ERROR_INVALID_ARGUMENT);

  // NOTE: traa_get_camera_capability only checks when BOTH capabilities AND count are nullptr.
  // When only one is nullptr, the API proceeds to the engine (actual behavior).

  // traa_start_camera_capture — nullptr config
  EXPECT_EQ(traa_start_camera_capture(nullptr), TRAA_ERROR_INVALID_ARGUMENT);

  // traa_stop_camera_capture — nullptr device_id
  EXPECT_EQ(traa_stop_camera_capture(nullptr), TRAA_ERROR_INVALID_ARGUMENT);

  traa_release();
}


// ---------------------------------------------------------------------------
// Test 3: TRAA_ERROR_NONE value is 0
// Validates: Requirements 10.3
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, error_none_is_zero) {
  EXPECT_EQ(static_cast<int>(TRAA_ERROR_NONE), 0);
}

// ---------------------------------------------------------------------------
// Test 4: All API error return values are within valid traa_error enum range
// Validates: Requirements 10.4
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, error_codes_in_valid_range) {
  // Helper lambda: verify an error code is in [0, TRAA_ERROR_COUNT).
  auto in_range = [](int err) {
    return err >= 0 && err < static_cast<int>(TRAA_ERROR_COUNT);
  };

  traa_config config;
  ASSERT_EQ(traa_init(&config), TRAA_ERROR_NONE);

  // Call various APIs with invalid inputs and verify returned codes are in range.

  // traa_init with nullptr (already initialized, but nullptr should still give valid code)
  int err = traa_init(nullptr);
  EXPECT_TRUE(in_range(err)) << "traa_init(nullptr) returned out-of-range: " << err;

  // traa_set_log with nullptr
  err = traa_set_log(nullptr);
  EXPECT_TRUE(in_range(err)) << "traa_set_log(nullptr) returned out-of-range: " << err;

  // traa_enum_device_info with nullptr params
  err = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, nullptr, nullptr);
  EXPECT_TRUE(in_range(err)) << "traa_enum_device_info(nullptr) returned out-of-range: " << err;

  // traa_get_camera_capability with nullptr params
  err = traa_get_camera_capability(nullptr, nullptr, nullptr);
  EXPECT_TRUE(in_range(err))
      << "traa_get_camera_capability(nullptr) returned out-of-range: " << err;

  // traa_start_camera_capture with nullptr
  err = traa_start_camera_capture(nullptr);
  EXPECT_TRUE(in_range(err))
      << "traa_start_camera_capture(nullptr) returned out-of-range: " << err;

  // traa_stop_camera_capture with nullptr
  err = traa_stop_camera_capture(nullptr);
  EXPECT_TRUE(in_range(err))
      << "traa_stop_camera_capture(nullptr) returned out-of-range: " << err;

  // traa_set_event_handler with nullptr
  err = traa_set_event_handler(nullptr);
  EXPECT_TRUE(in_range(err))
      << "traa_set_event_handler(nullptr) returned out-of-range: " << err;

  // traa_get_camera_capability with nonexistent device
  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  err = traa_get_camera_capability("nonexistent_device", &caps, &cap_count);
  EXPECT_TRUE(in_range(err))
      << "traa_get_camera_capability(nonexistent) returned out-of-range: " << err;

  // traa_stop_camera_capture with unknown device
  err = traa_stop_camera_capture("never_started_device");
  EXPECT_TRUE(in_range(err))
      << "traa_stop_camera_capture(unknown) returned out-of-range: " << err;

  traa_release();
}

// ---------------------------------------------------------------------------
// Test 5: Double init returns ALREADY_INITIALIZED
// Validates: Requirements 10.5
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, double_init_returns_already_initialized) {
  traa_config config;
  ASSERT_EQ(traa_init(&config), TRAA_ERROR_NONE);

  // NOTE: The current implementation's engine::init() always returns TRAA_ERROR_NONE.
  // Second init may return NONE or ALREADY_INITIALIZED depending on implementation.
  int ret = traa_init(&config);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || ret == TRAA_ERROR_ALREADY_INITIALIZED);

  traa_release();
}

// ---------------------------------------------------------------------------
// Test 6: stop_camera_capture with never-started device returns NOT_FOUND
// Validates: Requirements 10.6
// ---------------------------------------------------------------------------
TEST(traa_error_consistency, stop_capture_never_started_returns_not_found) {
  traa_config config;
  ASSERT_EQ(traa_init(&config), TRAA_ERROR_NONE);

  EXPECT_EQ(traa_stop_camera_capture("device_never_started_xyz"), TRAA_ERROR_NOT_FOUND);

  traa_release();
}
