#include <gtest/gtest.h>

#include <traa/traa.h>

#include <thread>

TEST(multi_thread_call, traa_init_release) {
  traa_config config;
  traa_event_handler event_handler;

  auto t1 = std::thread([&]() {
    for (int i = 0; i < 100; i++) {
      EXPECT_NO_THROW(traa_init(&config));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_set_event_handler(&event_handler));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_release());
    }
  });

  auto t2 = std::thread([&]() {
    for (int i = 0; i < 100; i++) {
      EXPECT_NO_THROW(traa_init(&config));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_set_event_handler(&event_handler));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_release());
    }
  });

  auto t3 = std::thread([&]() {
    for (int i = 0; i < 100; i++) {
      EXPECT_NO_THROW(traa_init(&config));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_set_event_handler(&event_handler));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_release());
    }
  });

  auto t4 = std::thread([&]() {
    for (int i = 0; i < 100; i++) {
      EXPECT_NO_THROW(traa_init(&config));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_set_event_handler(&event_handler));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      EXPECT_NO_THROW(traa_release());
    }
  });

  t1.join();
  t2.join();
  t3.join();
  t4.join();
}

class traa_engine_test : public testing::Test {
protected:
  void SetUp() override {
    traa_config config;

    // set the userdata.
    config.userdata = reinterpret_cast<traa_userdata>(0x12345678);

    // set the log config.
    config.log_config.log_file = "./traa.log";

    // set the event handler.
    config.event_handler.on_error = [](traa_userdata userdata, traa_error error_code,
                                       const char *context) {
      printf("userdata: %p, error_code: %d, context: %s\n", userdata, error_code, context);
    };

    // initialize the traa library.
    EXPECT_EQ(traa_init(&config), traa_error::TRAA_ERROR_NONE);
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