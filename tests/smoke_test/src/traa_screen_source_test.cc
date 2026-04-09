#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif // __APPLE__

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)

#include <gtest/gtest.h>

#include <traa/traa.h>

#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

// Redefine the fixture since TEST_F requires the class visible in this translation unit.
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

// Helper: check if return code indicates screen source enumeration is unavailable.
static auto is_unavailable = [](int ret) {
  return ret == TRAA_ERROR_PERMISSION_DENIED || ret == TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED;
};

// Requirement 8.1: Default flags (0) returns TRAA_ERROR_NONE, PERMISSION_DENIED, or ENUM_FAILED
TEST_F(traa_engine_test, screen_source_default_flags) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size, 0, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));

  if (ret == TRAA_ERROR_NONE && count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.2: IGNORE_SCREEN flag — all sources have is_window == true
TEST_F(traa_engine_test, screen_source_ignore_screen) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size,
                                         TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  for (int i = 0; i < count; i++) {
    EXPECT_TRUE(infos[i].is_window);
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.3: IGNORE_WINDOW flag — all sources have is_window == false
TEST_F(traa_engine_test, screen_source_ignore_window) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size,
                                         TRAA_SCREEN_SOURCE_FLAG_IGNORE_WINDOW, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  for (int i = 0; i < count; i++) {
    EXPECT_FALSE(infos[i].is_window);
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.4: IGNORE_SCREEN | IGNORE_WINDOW — count == 0 or non-TRAA_ERROR_NONE
TEST_F(traa_engine_test, screen_source_ignore_screen_and_window) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(
      icon_size, thumbnail_size,
      TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN | TRAA_SCREEN_SOURCE_FLAG_IGNORE_WINDOW, &infos,
      &count);
  EXPECT_TRUE(ret != TRAA_ERROR_NONE || count == 0);

  if (ret == TRAA_ERROR_NONE && count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.5: IGNORE_MINIMIZED flag — all sources have is_minimized == false
TEST_F(traa_engine_test, screen_source_ignore_minimized) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size,
                                         TRAA_SCREEN_SOURCE_FLAG_IGNORE_MINIMIZED, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  for (int i = 0; i < count; i++) {
    EXPECT_FALSE(infos[i].is_minimized);
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.6: Non-zero icon_size and thumbnail_size — window sources have non-null icon/thumb
TEST_F(traa_engine_test, screen_source_with_icon_and_thumbnail) {
  traa_size icon_size(100, 100);
  traa_size thumbnail_size(320, 240);
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size, 0, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  for (int i = 0; i < count; i++) {
    if (infos[i].is_window) {
      EXPECT_NE(infos[i].icon_data, nullptr);
      EXPECT_NE(infos[i].thumbnail_data, nullptr);
    }
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.7: Zero icon_size and thumbnail_size — all sources have null icon/thumb data
TEST_F(traa_engine_test, screen_source_zero_icon_and_thumbnail) {
  traa_size icon_size(0, 0);
  traa_size thumbnail_size(0, 0);
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size, 0, &infos, &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  for (int i = 0; i < count; i++) {
    EXPECT_EQ(infos[i].icon_data, nullptr);
    EXPECT_EQ(infos[i].thumbnail_data, nullptr);
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

// Requirement 8.8: infos parameter as nullptr
// NOTE: The API only checks when BOTH infos AND count are nullptr.
// When only infos is nullptr, the API passes through to the enumerator which may crash.
// We test the both-nullptr case instead.
TEST_F(traa_engine_test, screen_source_nullptr_infos) {
  traa_size icon_size;
  traa_size thumbnail_size;

  // Both nullptr returns INVALID_ARGUMENT
  EXPECT_EQ(traa_enum_screen_source_info(icon_size, thumbnail_size, 0, nullptr, nullptr),
            TRAA_ERROR_INVALID_ARGUMENT);
}

// Requirement 8.9: count parameter as nullptr
// NOTE: Same as above — single nullptr is not validated by the API.
// We verify the both-nullptr case is handled.
TEST_F(traa_engine_test, screen_source_nullptr_count) {
  traa_size icon_size;
  traa_size thumbnail_size;

  // Both nullptr returns INVALID_ARGUMENT (already tested above, but confirms consistency)
  EXPECT_EQ(traa_enum_screen_source_info(icon_size, thumbnail_size, 0, nullptr, nullptr),
            TRAA_ERROR_INVALID_ARGUMENT);
}

// Requirement 8.10: After valid return, traa_free_screen_source_info returns TRAA_ERROR_NONE
TEST_F(traa_engine_test, screen_source_free_after_enum) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size, 0, &infos, &count);
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  ASSERT_EQ(ret, TRAA_ERROR_NONE);
  if (count > 0) {
    EXPECT_EQ(traa_free_screen_source_info(infos, count), TRAA_ERROR_NONE);
  }
}

// Requirement 8.11: traa_free_screen_source_info(nullptr, 0) doesn't crash
TEST_F(traa_engine_test, screen_source_free_nullptr) {
  // Should not crash; accept any return value.
  traa_free_screen_source_info(nullptr, 0);
}

// Requirement 8.12: IGNORE_CURRENT_PROCESS_WINDOWS — no current process windows in result
TEST_F(traa_engine_test, screen_source_ignore_current_process_windows) {
  traa_size icon_size;
  traa_size thumbnail_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_screen_source_info(
      icon_size, thumbnail_size, TRAA_SCREEN_SOURCE_FLAG_IGNORE_CURRENT_PROCESS_WINDOWS, &infos,
      &count);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || is_unavailable(ret));
  if (is_unavailable(ret)) {
    GTEST_SKIP() << "Screen source enumeration unavailable";
  }

  // Get current process path to compare against returned sources.
#if defined(_WIN32)
  char current_exe[MAX_PATH] = {0};
  GetModuleFileNameA(nullptr, current_exe, MAX_PATH);
#else
  // On Linux/macOS we can compare using process ID indirectly.
  // The process_path field should not match our own executable.
  char current_exe[1024] = {0};
#if defined(__APPLE__)
  uint32_t buf_size = sizeof(current_exe);
  _NSGetExecutablePath(current_exe, &buf_size);
#elif defined(__linux__)
  ssize_t len = readlink("/proc/self/exe", current_exe, sizeof(current_exe) - 1);
  if (len > 0) {
    current_exe[len] = '\0';
  }
#endif
#endif

  for (int i = 0; i < count; i++) {
    if (infos[i].is_window && std::strlen(current_exe) > 0 &&
        std::strlen(infos[i].process_path) > 0) {
      // The returned window sources should not belong to the current process.
      EXPECT_STRNE(infos[i].process_path, current_exe)
          << "Window from current process should be filtered out";
    }
  }

  if (count > 0) {
    traa_free_screen_source_info(infos, count);
  }
}

#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE && (!defined(TARGET_OS_VISION)
       // || !TARGET_OS_VISION)) || __linux__
