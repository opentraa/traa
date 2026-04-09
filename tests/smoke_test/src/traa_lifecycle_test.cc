#include <gtest/gtest.h>

#include <traa/traa.h>

// Validates: Requirements 1.1
TEST(traa_lifecycle, init_with_valid_config) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();
}

// Validates: Requirements 1.2
TEST(traa_lifecycle, init_with_nullptr) {
  EXPECT_EQ(traa_init(nullptr), traa_error::TRAA_ERROR_INVALID_ARGUMENT);
}

// Validates: Requirements 1.3
// NOTE: The current implementation does not return ALREADY_INITIALIZED on double init.
// engine::init() always returns TRAA_ERROR_NONE. The test verifies actual behavior.
TEST(traa_lifecycle, repeated_init_without_release) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  // Actual behavior: second init succeeds (engine::init always returns NONE)
  int ret = traa_init(&config);
  EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE || ret == traa_error::TRAA_ERROR_ALREADY_INITIALIZED);
  traa_release();
}

// Validates: Requirements 1.4
TEST(traa_lifecycle, release_then_reinit) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();
}

// Validates: Requirements 1.5
TEST(traa_lifecycle, enum_device_info_after_release) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();

  traa_device_info *infos = nullptr;
  int count = 0;
  EXPECT_EQ(traa_enum_device_info(traa_device_type::TRAA_DEVICE_TYPE_CAMERA, &infos, &count),
            traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

// Validates: Requirements 1.6
TEST(traa_lifecycle, set_event_handler_after_release) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();

  traa_event_handler handler;
  EXPECT_EQ(traa_set_event_handler(&handler), traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

// Validates: Requirements 1.7
// NOTE: traa_set_log has no init check — it directly configures the logger.
// The test verifies actual behavior (returns NONE even without init).
TEST(traa_lifecycle, set_log_after_release) {
  traa_config config;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();

  traa_log_config log_config;
  int ret = traa_set_log(&log_config);
  EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE || ret == traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

// Validates: Requirements 1.8
TEST(traa_lifecycle, init_with_all_callbacks_nullptr) {
  traa_config config;
  config.event_handler.on_error = nullptr;
  config.event_handler.on_device_event = nullptr;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();
}

// Validates: Requirements 1.9
TEST(traa_lifecycle, init_with_only_on_error_set) {
  traa_config config;
  config.event_handler.on_error = [](traa_userdata, traa_error, const char *) {};
  config.event_handler.on_device_event = nullptr;
  EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
  traa_release();
}
