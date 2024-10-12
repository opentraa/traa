#include <gtest/gtest.h>

#include <traa/traa.h>

#include <thread>

TEST(multi_thread_call, traa_init_release) {
  traa_config config;
  traa_event_handler event_handler;

  auto worker = [&]() {
    for (int i = 0; i < 100; i++) {
      traa_init(&config);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      traa_set_event_handler(&event_handler);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      traa_release();
    }
  };

  unsigned int thread_count = std::thread::hardware_concurrency();

  std::vector<std::thread> threads;
  for (unsigned int i = 0; i < thread_count; i++) {
    threads.push_back(std::thread(worker));
  }

  for (auto &t : threads) {
    if (t.joinable())
      t.join();
  }
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

  void TearDown() override { traa_release(); }
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

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
TEST_F(traa_engine_test, traa_enum_and_free_screen_source_info) {
  auto output_screen_source_info = [](traa_screen_source_info *info) {
    // print all
    printf("info: id: %lld, screen_id: %lld, is_window: %d, is_minimized: %d, is_maximized: %d, "
           "rect: (%d, %d, %d, %d), title: %s, process_path:%s, icon_size: (%d, "
           "%d), "
           "icon_data: %p, thumbnail_size: (%d, %d), thumbnail_data: %p\n",
           info->id, info->screen_id, info->is_window, info->is_minimized, info->is_maximized,
           info->rect.left, info->rect.top, info->rect.right, info->rect.bottom, info->title,
           info->process_path, info->icon_size.width, info->icon_size.height, info->icon_data,
           info->thumbnail_size.width, info->thumbnail_size.height, info->thumbnail_data);

    // print id and title
    // printf("info: id: %lld, title: %s\n", info->id, info->title);

    // print id 、title and process_path
    /*printf("info: id: %lld, title: %s, process_path: %s\n", info->id, info->title,
           info->process_path);*/
  };

  // enum without icon_size and thumbnail_size.
  {
    traa_size icon_size;
    traa_size thumbnail_size;
    unsigned int external_flags = 0;

    traa_screen_source_info *infos = nullptr;
    int count = 0;

    EXPECT_TRUE(traa_enum_screen_source_info(icon_size, thumbnail_size, external_flags, &infos,
                                             &count) == traa_error::TRAA_ERROR_NONE);

    // expect the infos and count are not nullptr.
    EXPECT_NE(infos, nullptr);
    EXPECT_NE(count, 0);

    // check the infos.
    for (int i = 0; i < count; i++) {
      EXPECT_GE(infos[i].id, 0);
      if (infos[i].is_window) {
        EXPECT_GE(infos[i].screen_id, TRAA_FULLSCREEN_SCREEN_ID);
        EXPECT_GT(infos[i].rect.right - infos[i].rect.left, 0);
        EXPECT_GT(infos[i].rect.bottom - infos[i].rect.top, 0);
        EXPECT_EQ(infos[i].icon_size.width, 0);
        EXPECT_EQ(infos[i].icon_size.height, 0);
        EXPECT_TRUE(std::strlen(infos[i].title) > 0);
        EXPECT_TRUE(std::strlen(infos[i].process_path) > 0);
        EXPECT_EQ(infos[i].icon_data, nullptr);
      }

      EXPECT_EQ(infos[i].thumbnail_size.width, 0);
      EXPECT_EQ(infos[i].thumbnail_size.height, 0);
      EXPECT_EQ(infos[i].thumbnail_data, nullptr);

      output_screen_source_info(&infos[i]);
    }

    // free the infos.
    EXPECT_TRUE(traa_free_screen_source_info(infos, count) == traa_error::TRAA_ERROR_NONE);
  }

  // enum with icon_size and thumbnail_size
  {
    traa_size icon_size(100, 100);
    traa_size thumbnail_size(100, 100);
    unsigned int external_flags = 0;

    traa_screen_source_info *infos = nullptr;
    int count = 0;

    EXPECT_TRUE(traa_enum_screen_source_info(icon_size, thumbnail_size, external_flags, &infos,
                                             &count) == traa_error::TRAA_ERROR_NONE);

    // expect the infos and count are not nullptr.
    EXPECT_NE(infos, nullptr);
    EXPECT_NE(count, 0);

    // check the infos.
    for (int i = 0; i < count; i++) {
      EXPECT_GE(infos[i].id, 0);
      if (infos[i].is_window) {
        EXPECT_GE(infos[i].screen_id, TRAA_FULLSCREEN_SCREEN_ID);
        EXPECT_GT(infos[i].rect.right - infos[i].rect.left, 0);
        EXPECT_GT(infos[i].rect.bottom - infos[i].rect.top, 0);
        EXPECT_EQ(infos[i].icon_size.width, icon_size.width);
        EXPECT_EQ(infos[i].icon_size.height, icon_size.height);
        EXPECT_TRUE(std::strlen(infos[i].title) > 0);
        EXPECT_TRUE(std::strlen(infos[i].process_path) > 0);
        EXPECT_NE(infos[i].icon_data, nullptr);
      }

      EXPECT_EQ(infos[i].thumbnail_size.width, thumbnail_size.width);
      EXPECT_EQ(infos[i].thumbnail_size.height, thumbnail_size.height);
      EXPECT_NE(infos[i].thumbnail_data, nullptr);

      output_screen_source_info(&infos[i]);
    }

    // free the infos.
    EXPECT_TRUE(traa_free_screen_source_info(infos, count) == traa_error::TRAA_ERROR_NONE);
  }
}
#endif