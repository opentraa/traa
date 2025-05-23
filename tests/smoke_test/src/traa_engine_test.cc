#include "simple_window/simple_window.h"

#include <gtest/gtest.h>

#include <traa/traa.h>

#include <cstring>
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

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)
TEST_F(traa_engine_test, traa_enum_and_free_screen_source_info) {
  auto output_screen_source_info = [](traa_screen_source_info *info) {
    // print all
    printf(
        "info: id: %lld, screen_id: %lld, is_window: %d, is_minimized: %d, is_maximized: %d, "
        "is_primary: %d, rect: (%d, %d, %d, %d), title: %s, process_path:%s, icon_size: (%d, %d), "
        "icon_data: %p, thumbnail_size: (%d, %d), thumbnail_data: %p\n",
        static_cast<long long>(info->id), static_cast<long long>(info->screen_id), info->is_window,
        info->is_minimized, info->is_maximized, info->is_primary, info->rect.left, info->rect.top,
        info->rect.right, info->rect.bottom, info->title, info->process_path, info->icon_size.width,
        info->icon_size.height, info->icon_data, info->thumbnail_size.width,
        info->thumbnail_size.height, info->thumbnail_data);

    // print id and title
    // printf("info: id: %lld, title: %s\n", static_cast<long long>(info->id), info->title);

    // print id 、title and process_path
    /*printf("info: id: %lld, title: %s, process_path: %s\n", static_cast<long long>(info->id),
       info->title, info->process_path);*/
  };

  auto simple_window = traa::base::simple_window::create("simple_window", 300, 300);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // enum without icon_size and thumbnail_size.
  {
    traa_size icon_size;
    traa_size thumbnail_size;
    unsigned int external_flags = TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN;

    traa_screen_source_info *infos = nullptr;
    int count = 0;

    int ret =
        traa_enum_screen_source_info(icon_size, thumbnail_size, external_flags, &infos, &count);
    EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE ||
                ret == traa_error::TRAA_ERROR_PERMISSION_DENIED ||
                ret == traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED);
    if (ret == traa_error::TRAA_ERROR_PERMISSION_DENIED) {
      printf("Permission denied\n");
      return;
    }

    // in linux, if is running under wayland, the ret is TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED.
    if (ret == traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED) {
      printf("Enum screen source info failed\n");
      return;
    }

    // expect the infos and count are not nullptr.
    EXPECT_NE(infos, nullptr);
    EXPECT_NE(count, 0);

    // check the infos.
    for (int i = 0; i < count; i++) {
      // expect all the infos are window, coz we set the external_flags to
      // TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN.
      EXPECT_TRUE(infos[i].is_window);

      EXPECT_GE(infos[i].id, 0);
#if defined(_WIN32)
      EXPECT_GE(infos[i].screen_id, TRAA_FULLSCREEN_SCREEN_ID);
#endif // _WIN32
      EXPECT_GT(infos[i].rect.right - infos[i].rect.left, 0);
      EXPECT_GT(infos[i].rect.bottom - infos[i].rect.top, 0);
      EXPECT_EQ(infos[i].icon_size.width, 0);
      EXPECT_EQ(infos[i].icon_size.height, 0);
      EXPECT_TRUE(std::strlen(infos[i].title) > 0);
      EXPECT_TRUE(std::strlen(infos[i].process_path) > 0);
      EXPECT_EQ(infos[i].icon_data, nullptr);

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
    traa_size thumbnail_size(1920, 1080);
    unsigned int external_flags = 0;

    traa_screen_source_info *infos = nullptr;
    int count = 0;

    int ret =
        traa_enum_screen_source_info(icon_size, thumbnail_size, external_flags, &infos, &count);
    EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE ||
                ret == traa_error::TRAA_ERROR_PERMISSION_DENIED ||
                ret == traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED);
    if (ret == traa_error::TRAA_ERROR_PERMISSION_DENIED) {
      printf("Permission denied\n");
      return;
    }

    // expect the infos and count are not nullptr.
    EXPECT_NE(infos, nullptr);
    EXPECT_NE(count, 0);

    // check the infos.
    for (int i = 0; i < count; i++) {
      EXPECT_GE(infos[i].id, 0);
      if (infos[i].is_window) {
#if defined(_WIN32)
        EXPECT_GE(infos[i].screen_id, TRAA_FULLSCREEN_SCREEN_ID);
#endif // _WIN32
        EXPECT_GT(infos[i].rect.right - infos[i].rect.left, 0);
        EXPECT_GT(infos[i].rect.bottom - infos[i].rect.top, 0);
        EXPECT_GE(icon_size.width * icon_size.height,
                  infos[i].icon_size.width * infos[i].icon_size.height);
        EXPECT_TRUE(std::strlen(infos[i].title) > 0);
        EXPECT_TRUE(std::strlen(infos[i].process_path) > 0);
        EXPECT_NE(infos[i].icon_data, nullptr);
      }

      EXPECT_NE(infos[i].thumbnail_data, nullptr);
      EXPECT_GE(thumbnail_size.width * thumbnail_size.height,
                infos[i].thumbnail_size.width * infos[i].thumbnail_size.height);

      output_screen_source_info(&infos[i]);
    }

    // free the infos.
    EXPECT_TRUE(traa_free_screen_source_info(infos, count) == traa_error::TRAA_ERROR_NONE);
  }
}

// only available on windows and macos
#if defined(_WIN32) || defined(__APPLE__)
TEST_F(traa_engine_test, traa_create_snapshot) {
  // Create a window first to ensure there is an available window source
  auto simple_window = traa::base::simple_window::create("simple_window", 300, 300);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // get the available source IDs (screen and window)
  int64_t screen_source_id = -1;
  int64_t window_source_id = -1;

  {
    traa_screen_source_info *infos = nullptr;
    int count = 0;

    int ret = traa_enum_screen_source_info({0, 0}, {0, 0}, 0, &infos, &count);
    EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE ||
                ret == traa_error::TRAA_ERROR_PERMISSION_DENIED ||
                ret == traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED);

    if (ret == traa_error::TRAA_ERROR_NONE && count > 0) {
      // find a screen and a window to test
      for (int i = 0; i < count; i++) {
        if (!infos[i].is_window && screen_source_id == -1) {
          screen_source_id = infos[i].id;
        } else if (infos[i].is_window && window_source_id == -1) {
          window_source_id = infos[i].id;
        }

        if (screen_source_id != -1 && window_source_id != -1) {
          break;
        }
      }

      printf("Found screen_source_id: %lld, window_source_id: %lld\n",
             static_cast<long long>(screen_source_id), static_cast<long long>(window_source_id));
    }

    if (infos != nullptr) {
      traa_free_screen_source_info(infos, count);
    }
  }

  // test 1: invalid source ID
  {
    uint8_t *data = nullptr;
    int data_size = 0;
    traa_size snapshot_size(800, 600);
    traa_size actual_size;
    int ret = traa_create_snapshot(-1, snapshot_size, &data, &data_size, &actual_size);
    EXPECT_EQ(ret, traa_error::TRAA_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(data_size, 0);
    EXPECT_EQ(actual_size.width, 0);
    EXPECT_EQ(actual_size.height, 0);
  }

  // test 2: invalid input - nullptr
  {
    traa_size snapshot_size(800, 600);

    int ret = traa_create_snapshot(1, snapshot_size, nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, traa_error::TRAA_ERROR_INVALID_ARGUMENT);
  }

  // test 3: valid input - use screen source ID
  if (screen_source_id != -1) {
    uint8_t *data = nullptr;
    int data_size = 0;
    traa_size snapshot_size(800, 600);
    traa_size actual_size;

    int ret =
        traa_create_snapshot(screen_source_id, snapshot_size, &data, &data_size, &actual_size);
    EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE ||
                ret == traa_error::TRAA_ERROR_PERMISSION_DENIED);

    if (ret == traa_error::TRAA_ERROR_NONE) {
      printf("data_size: %d, actual_size: %d, %d\n", data_size, actual_size.width,
             actual_size.height);
      EXPECT_NE(data, nullptr);
      EXPECT_GT(data_size, 0);
      EXPECT_GT(actual_size.width, 0);
      EXPECT_GT(actual_size.height, 0);

      // if get the data, should free it
      traa_free_snapshot(data);
    } else if (ret == traa_error::TRAA_ERROR_PERMISSION_DENIED) {
      printf("Permission denied for screen capture\n");
    }
  } else {
    printf("No valid screen source ID found, skipping screen snapshot test\n");
  }

  // test 4: valid input - use window source ID
  if (window_source_id != -1) {
    uint8_t *data = nullptr;
    int data_size = 0;
    traa_size snapshot_size(800, 600);
    traa_size actual_size;
    int ret =
        traa_create_snapshot(window_source_id, snapshot_size, &data, &data_size, &actual_size);
    EXPECT_TRUE(ret == traa_error::TRAA_ERROR_NONE ||
                ret == traa_error::TRAA_ERROR_PERMISSION_DENIED);

    if (ret == traa_error::TRAA_ERROR_NONE) {
      printf("data_size: %d, actual_size: %d, %d\n", data_size, actual_size.width,
             actual_size.height);
      EXPECT_NE(data, nullptr);
      EXPECT_GT(data_size, 0);
      EXPECT_GT(actual_size.width, 0);
      EXPECT_GT(actual_size.height, 0);

      // if get the data, should free it
      traa_free_snapshot(data);
    } else if (ret == traa_error::TRAA_ERROR_PERMISSION_DENIED) {
      printf("Permission denied for window capture\n");
    }
  } else {
    printf("No valid window source ID found, skipping window snapshot test\n");
  }
}
#endif // _WIN32 || __APPLE__
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE && (!defined(TARGET_OS_VISION)
       // || !TARGET_OS_VISION)) || __linux__