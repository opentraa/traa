#include <gtest/gtest.h>

#include <traa/traa.h>

#include <cstdio>

// =============================================================================
// LOG LEVEL tests — no fixture needed (traa_set_log_level is stateless)
// =============================================================================

// Validates: Requirements 3.1-3.7
TEST(traa_log_level_test, all_log_levels_no_crash) {
  const traa_log_level levels[] = {
      TRAA_LOG_LEVEL_TRACE, TRAA_LOG_LEVEL_DEBUG, TRAA_LOG_LEVEL_INFO, TRAA_LOG_LEVEL_WARN,
      TRAA_LOG_LEVEL_ERROR, TRAA_LOG_LEVEL_FATAL, TRAA_LOG_LEVEL_OFF,
  };

  for (auto level : levels) {
    traa_set_log_level(level);
  }
}

// Validates: Requirement 3.8
TEST(traa_log_level_test, set_log_level_before_init_no_crash) {
  // traa_set_log_level is stateless and can be called at any time, even before traa_init.
  traa_set_log_level(TRAA_LOG_LEVEL_DEBUG);
  traa_set_log_level(TRAA_LOG_LEVEL_OFF);
  traa_set_log_level(TRAA_LOG_LEVEL_TRACE);
}

// Validates: Requirement 3.9
TEST(traa_log_level_test, switch_multiple_levels_consecutively_no_crash) {
  traa_set_log_level(TRAA_LOG_LEVEL_TRACE);
  traa_set_log_level(TRAA_LOG_LEVEL_WARN);
  traa_set_log_level(TRAA_LOG_LEVEL_OFF);
  traa_set_log_level(TRAA_LOG_LEVEL_DEBUG);
  traa_set_log_level(TRAA_LOG_LEVEL_FATAL);
  traa_set_log_level(TRAA_LOG_LEVEL_INFO);
  traa_set_log_level(TRAA_LOG_LEVEL_ERROR);
}

// =============================================================================
// LOG CONFIG tests — use traa_engine_test fixture (requires traa_init)
// =============================================================================

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

// Validates: Requirement 4.1
TEST_F(traa_engine_test, traa_set_log_valid_path) {
  traa_log_config log_config("./traa_test.log");
  EXPECT_EQ(traa_set_log(&log_config), TRAA_ERROR_NONE);
}

// Validates: Requirement 4.2
TEST_F(traa_engine_test, traa_set_log_nullptr_config) {
  EXPECT_EQ(traa_set_log(nullptr), TRAA_ERROR_INVALID_ARGUMENT);
}

// Validates: Requirement 4.3
TEST_F(traa_engine_test, traa_set_log_nullptr_log_file) {
  traa_log_config log_config; // log_file defaults to nullptr
  EXPECT_EQ(traa_set_log(&log_config), TRAA_ERROR_NONE);
}

// Validates: Requirement 4.4
TEST_F(traa_engine_test, traa_set_log_nonexistent_directory) {
  traa_log_config log_config("./nonexistent_dir_12345/traa.log");
  int ret = traa_set_log(&log_config);
  // The library may either return an error or auto-create the directory.
  // We just verify it doesn't crash and returns a valid error code.
  EXPECT_GE(ret, 0);
  EXPECT_LT(ret, TRAA_ERROR_COUNT);
}

// Validates: Requirement 4.5, 4.6
TEST_F(traa_engine_test, traa_set_log_different_levels) {
  const traa_log_level levels[] = {
      TRAA_LOG_LEVEL_TRACE, TRAA_LOG_LEVEL_DEBUG, TRAA_LOG_LEVEL_INFO, TRAA_LOG_LEVEL_WARN,
      TRAA_LOG_LEVEL_ERROR, TRAA_LOG_LEVEL_FATAL, TRAA_LOG_LEVEL_OFF,
  };

  for (auto level : levels) {
    traa_log_config log_config("./traa_test.log", 1024 * 1024 * 2, 3, level);
    EXPECT_EQ(traa_set_log(&log_config), TRAA_ERROR_NONE);
  }
}
