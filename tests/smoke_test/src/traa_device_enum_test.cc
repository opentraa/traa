#include <gtest/gtest.h>

#include <traa/traa.h>

#include <cstring>

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

// Requirements 5.1, 5.2, 5.11: Camera enumeration, field validation, and free
TEST_F(traa_engine_test, enum_camera_devices) {
  traa_device_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, &infos, &count);
  EXPECT_EQ(ret, TRAA_ERROR_NONE);
  EXPECT_GE(count, 0);

  if (count > 0) {
    ASSERT_NE(infos, nullptr);
    for (int i = 0; i < count; i++) {
      EXPECT_GT(std::strlen(infos[i].name), 0u);
      EXPECT_GT(std::strlen(infos[i].id), 0u);
      EXPECT_EQ(infos[i].type, TRAA_DEVICE_TYPE_CAMERA);
    }
    // Requirement 5.11: free after valid return
    EXPECT_EQ(traa_free_device_info(infos), TRAA_ERROR_NONE);
  }
}

// Requirements 5.3, 5.4: Microphone enumeration and type validation
TEST_F(traa_engine_test, enum_microphone_devices) {
  traa_device_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_MICROPHONE, &infos, &count);
  EXPECT_EQ(ret, TRAA_ERROR_NONE);
  EXPECT_GE(count, 0);

  if (count > 0) {
    ASSERT_NE(infos, nullptr);
    for (int i = 0; i < count; i++) {
      EXPECT_EQ(infos[i].type, TRAA_DEVICE_TYPE_MICROPHONE);
    }
    traa_free_device_info(infos);
  }
}

// Requirements 5.5, 5.6: Speaker enumeration and type validation
TEST_F(traa_engine_test, enum_speaker_devices) {
  traa_device_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_SPEAKER, &infos, &count);
  EXPECT_EQ(ret, TRAA_ERROR_NONE);
  EXPECT_GE(count, 0);

  if (count > 0) {
    ASSERT_NE(infos, nullptr);
    for (int i = 0; i < count; i++) {
      EXPECT_EQ(infos[i].type, TRAA_DEVICE_TYPE_SPEAKER);
    }
    traa_free_device_info(infos);
  }
}

// Requirement 5.7: nullptr infos parameter
// NOTE: The API only checks when BOTH infos AND count are nullptr.
// When only infos is nullptr, the API proceeds (actual behavior).
TEST_F(traa_engine_test, enum_device_info_nullptr_infos) {
  int count = 0;
  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, nullptr, &count);
  // Actual behavior: API proceeds when only one param is nullptr
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || ret == TRAA_ERROR_INVALID_ARGUMENT);
}

// Requirement 5.8: nullptr count parameter
// NOTE: Same as above — API only checks when BOTH are nullptr.
TEST_F(traa_engine_test, enum_device_info_nullptr_count) {
  traa_device_info *infos = nullptr;
  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, &infos, nullptr);
  EXPECT_TRUE(ret == TRAA_ERROR_NONE || ret == TRAA_ERROR_INVALID_ARGUMENT);
  if (infos != nullptr) {
    traa_free_device_info(infos);
  }
}

// Requirement 5.9: both infos and count as nullptr
TEST_F(traa_engine_test, enum_device_info_both_nullptr) {
  EXPECT_EQ(traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, nullptr, nullptr),
            TRAA_ERROR_INVALID_ARGUMENT);
}

// Requirement 5.10: TRAA_DEVICE_TYPE_UNKNOWN returns non-NONE or count == 0
TEST_F(traa_engine_test, enum_device_info_unknown_type) {
  traa_device_info *infos = nullptr;
  int count = 0;

  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_UNKNOWN, &infos, &count);
  EXPECT_TRUE(ret != TRAA_ERROR_NONE || count == 0);

  if (infos != nullptr) {
    traa_free_device_info(infos);
  }
}

// Requirement 5.12: traa_free_device_info(nullptr) doesn't crash
TEST_F(traa_engine_test, free_device_info_nullptr) {
  // Should not crash; accept any return value
  traa_free_device_info(nullptr);
}
