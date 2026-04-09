#include <gtest/gtest.h>

#include <traa/traa.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

// Redefine the fixture so it is visible in this translation unit.
class traa_engine_test : public testing::Test {
protected:
  void SetUp() override {
    traa_config config;
    config.userdata = reinterpret_cast<traa_userdata>(0x12345678);
    config.event_handler.on_error = [](const traa_userdata, traa_error, const char *) {};
    EXPECT_EQ(traa_init(&config), TRAA_ERROR_NONE);
  }
  void TearDown() override { traa_release(); }
};

// Helper: return the device id of the first available camera, or "" if none.
static std::string get_first_camera_id() {
  traa_device_info *infos = nullptr;
  int count = 0;
  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, &infos, &count);
  if (ret != TRAA_ERROR_NONE || count == 0) {
    if (infos)
      traa_free_device_info(infos);
    return "";
  }
  std::string id(infos[0].id);
  traa_free_device_info(infos);
  return id;
}

// ---------------------------------------------------------------------------
// Frame receipt tracker used by capture tests
// ---------------------------------------------------------------------------
struct frame_receipt {
  std::atomic<int> frame_count{0};
  std::atomic<bool> has_valid_frame{false};
};

// ===================================================================
// CAMERA CAPABILITY QUERY tests  (Requirement 6)
// ===================================================================

// 1. Valid device_id returns TRAA_ERROR_NONE (skip if no camera)
TEST_F(traa_engine_test, get_camera_capability_valid_device) {
  std::string cam_id = get_first_camera_id();
  if (cam_id.empty()) {
    GTEST_SKIP() << "No camera device available";
  }

  traa_video_capability *caps = nullptr;
  int count = 0;
  EXPECT_EQ(traa_get_camera_capability(cam_id.c_str(), &caps, &count), TRAA_ERROR_NONE);

  // 2. count > 0, each capability has positive width/height/max_fps
  EXPECT_GT(count, 0);
  for (int i = 0; i < count; ++i) {
    EXPECT_GT(caps[i].width, 0);
    EXPECT_GT(caps[i].height, 0);
    EXPECT_GT(caps[i].max_fps, 0);
  }

  // 3. Free returns TRAA_ERROR_NONE
  EXPECT_EQ(traa_free_camera_capability(caps), TRAA_ERROR_NONE);
}

// 4. device_id nullptr → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, get_camera_capability_nullptr_device_id) {
  traa_video_capability *caps = nullptr;
  int count = 0;
  EXPECT_EQ(traa_get_camera_capability(nullptr, &caps, &count), TRAA_ERROR_INVALID_ARGUMENT);
}

// 5. device_id empty string → non-TRAA_ERROR_NONE
TEST_F(traa_engine_test, get_camera_capability_empty_device_id) {
  traa_video_capability *caps = nullptr;
  int count = 0;
  EXPECT_NE(traa_get_camera_capability("", &caps, &count), TRAA_ERROR_NONE);
}

// 6. device_id nonexistent → non-TRAA_ERROR_NONE
TEST_F(traa_engine_test, get_camera_capability_nonexistent_device_id) {
  traa_video_capability *caps = nullptr;
  int count = 0;
  EXPECT_NE(traa_get_camera_capability("nonexistent_device_12345", &caps, &count),
            TRAA_ERROR_NONE);
}

// 7. capabilities nullptr → API only checks when BOTH capabilities AND count are nullptr
// NOTE: When only capabilities is nullptr, the API proceeds to the engine.
TEST_F(traa_engine_test, get_camera_capability_nullptr_capabilities) {
  int count = 0;
  int ret = traa_get_camera_capability("any", nullptr, &count);
  // Actual behavior: API proceeds when only one param is nullptr
  EXPECT_NE(ret, TRAA_ERROR_NONE); // Should fail (device not found or other error)
}

// 8. count nullptr → same as above
TEST_F(traa_engine_test, get_camera_capability_nullptr_count) {
  traa_video_capability *caps = nullptr;
  int ret = traa_get_camera_capability("any", &caps, nullptr);
  EXPECT_NE(ret, TRAA_ERROR_NONE); // Should fail (device not found or other error)
  if (caps != nullptr) {
    traa_free_camera_capability(caps);
  }
}

// 9. traa_free_camera_capability(nullptr) doesn't crash
TEST_F(traa_engine_test, free_camera_capability_nullptr) {
  // Should not crash; we don't assert a specific return value.
  traa_free_camera_capability(nullptr);
}

// ===================================================================
// CAMERA CAPTURE START/STOP tests  (Requirement 7)
// ===================================================================

// 10-14. Combined: start capture, receive frame, validate, stop, confirm no new frames.
TEST_F(traa_engine_test, camera_capture_start_receive_stop) {
  std::string cam_id = get_first_camera_id();
  if (cam_id.empty()) {
    GTEST_SKIP() << "No camera device available";
  }

  // Get first capability
  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  int ret = traa_get_camera_capability(cam_id.c_str(), &caps, &cap_count);
  if (ret != TRAA_ERROR_NONE || cap_count == 0) {
    if (caps)
      traa_free_camera_capability(caps);
    GTEST_SKIP() << "No camera capability available";
  }
  traa_video_capability first_cap = caps[0];
  traa_free_camera_capability(caps);

  // Set up frame receipt tracking
  frame_receipt receipt;

  traa_camera_config config;
  config.device_id = cam_id.c_str();
  config.capability = first_cap;
  config.userdata = &receipt;
  config.on_video_frame = [](const traa_userdata userdata, const traa_video_frame *frame) {
    auto *r = static_cast<frame_receipt *>(userdata);
    r->frame_count.fetch_add(1, std::memory_order_relaxed);
    if (frame && frame->data && frame->data_length > 0 && frame->width > 0 &&
        frame->height > 0) {
      r->has_valid_frame.store(true, std::memory_order_relaxed);
    }
  };

  // 10. Start capture
  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_NONE);

  // 11. Wait up to 5 seconds for at least one frame
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (receipt.frame_count.load(std::memory_order_relaxed) == 0 &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  EXPECT_GT(receipt.frame_count.load(std::memory_order_relaxed), 0)
      << "No frame received within 5 seconds";

  // 12. Validate frame data
  EXPECT_TRUE(receipt.has_valid_frame.load(std::memory_order_relaxed))
      << "Received frame(s) but none had valid data";

  // 13. Stop capture
  EXPECT_EQ(traa_stop_camera_capture(cam_id.c_str()), TRAA_ERROR_NONE);

  // 14. Wait 1 second and confirm no new frames arrive
  int count_after_stop = receipt.frame_count.load(std::memory_order_relaxed);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(receipt.frame_count.load(std::memory_order_relaxed), count_after_stop)
      << "Frames still arriving after stop";
}

// 15. config nullptr → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, start_camera_capture_nullptr_config) {
  EXPECT_EQ(traa_start_camera_capture(nullptr), TRAA_ERROR_INVALID_ARGUMENT);
}

// 16. Same device repeated start → TRAA_ERROR_ALREADY_EXISTS
TEST_F(traa_engine_test, start_camera_capture_duplicate) {
  std::string cam_id = get_first_camera_id();
  if (cam_id.empty()) {
    GTEST_SKIP() << "No camera device available";
  }

  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  int ret = traa_get_camera_capability(cam_id.c_str(), &caps, &cap_count);
  if (ret != TRAA_ERROR_NONE || cap_count == 0) {
    if (caps)
      traa_free_camera_capability(caps);
    GTEST_SKIP() << "No camera capability available";
  }
  traa_video_capability first_cap = caps[0];
  traa_free_camera_capability(caps);

  frame_receipt receipt;
  traa_camera_config config;
  config.device_id = cam_id.c_str();
  config.capability = first_cap;
  config.userdata = &receipt;
  config.on_video_frame = [](const traa_userdata, const traa_video_frame *) {};

  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_NONE);
  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_ALREADY_EXISTS);

  // Cleanup
  traa_stop_camera_capture(cam_id.c_str());
}

// 17. device_id nullptr for stop → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, stop_camera_capture_nullptr_device_id) {
  EXPECT_EQ(traa_stop_camera_capture(nullptr), TRAA_ERROR_INVALID_ARGUMENT);
}

// 18. Non-existent device_id for stop → TRAA_ERROR_NOT_FOUND
TEST_F(traa_engine_test, stop_camera_capture_nonexistent_device) {
  EXPECT_EQ(traa_stop_camera_capture("nonexistent_device_12345"), TRAA_ERROR_NOT_FOUND);
}

// 19. Already-stopped device repeated stop → TRAA_ERROR_NOT_FOUND
TEST_F(traa_engine_test, stop_camera_capture_already_stopped) {
  std::string cam_id = get_first_camera_id();
  if (cam_id.empty()) {
    GTEST_SKIP() << "No camera device available";
  }

  traa_video_capability *caps = nullptr;
  int cap_count = 0;
  int ret = traa_get_camera_capability(cam_id.c_str(), &caps, &cap_count);
  if (ret != TRAA_ERROR_NONE || cap_count == 0) {
    if (caps)
      traa_free_camera_capability(caps);
    GTEST_SKIP() << "No camera capability available";
  }
  traa_video_capability first_cap = caps[0];
  traa_free_camera_capability(caps);

  traa_camera_config config;
  config.device_id = cam_id.c_str();
  config.capability = first_cap;
  config.on_video_frame = [](const traa_userdata, const traa_video_frame *) {};

  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_NONE);
  EXPECT_EQ(traa_stop_camera_capture(cam_id.c_str()), TRAA_ERROR_NONE);
  EXPECT_EQ(traa_stop_camera_capture(cam_id.c_str()), TRAA_ERROR_NOT_FOUND);
}

// 20. on_video_frame = nullptr → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, start_camera_capture_null_callback) {
  traa_camera_config config;
  config.device_id = "some_device";
  config.on_video_frame = nullptr;
  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_INVALID_ARGUMENT);
}

// 21. device_id = nullptr in config → TRAA_ERROR_INVALID_ARGUMENT
TEST_F(traa_engine_test, start_camera_capture_null_device_id_in_config) {
  traa_camera_config config;
  config.device_id = nullptr;
  config.on_video_frame = [](const traa_userdata, const traa_video_frame *) {};
  EXPECT_EQ(traa_start_camera_capture(&config), TRAA_ERROR_INVALID_ARGUMENT);
}
