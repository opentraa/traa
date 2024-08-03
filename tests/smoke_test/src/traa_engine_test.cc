#include <gtest/gtest.h>

#include <traa/traa.h>

class traa_engine_test : public testing::Test {
private:
  static void on_error(traa_userdata userdata, traa_error error_code, const char *context) {
    printf("userdata: %p, error_code: %d, context: %s\n", userdata, error_code, context);
  }

protected:
  void SetUp() override {
    traa_config config;

    // set the userdata.
    config.userdata = reinterpret_cast<traa_userdata>(0x12345678);

    // set the log config.
    config.log_config.log_file = "./traa.log";

    // set the event handler.
    config.event_handler.on_error = on_error;

    // initialize the traa library.
    EXPECT_TRUE(traa_init(&config) == traa_error::TRAA_ERROR_NONE);
  }

  void TearDown() override { EXPECT_NO_THROW(traa_release()); }
};

TEST_F(traa_engine_test, traa_set_log_level) {
  traa_set_log_level(traa_log_level::TRAA_LOG_LEVEL_DEBUG);
  traa_set_log_level(traa_log_level::TRAA_LOG_LEVEL_INFO);
  traa_set_log_level(traa_log_level::TRAA_LOG_LEVEL_WARN);
  traa_set_log_level(traa_log_level::TRAA_LOG_LEVEL_ERROR);
  traa_set_log_level(traa_log_level::TRAA_LOG_LEVEL_FATAL);
}

TEST_F(traa_engine_test, traa_set_log) {
  traa_log_config log_config;

  log_config.log_file = "./traa.log";
  log_config.level = traa_log_level::TRAA_LOG_LEVEL_DEBUG;

  EXPECT_TRUE(traa_set_log(&log_config) == traa_error::TRAA_ERROR_NONE);
}