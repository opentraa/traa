#ifndef TRAA_MAIN_ENGINE_H_
#define TRAA_MAIN_ENGINE_H_

#include <traa/traa.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "base/disallow.h"
#include "base/thread/callback.h"

namespace traa {
namespace base {
class desktop_capturer;
class device_info_impl;
class video_capture_impl;
class video_frame_callback;
} // namespace base

namespace main {

class engine : public base::support_weak_callback {
  DISALLOW_COPY_AND_ASSIGN(engine);

public:
  engine();
  ~engine();

  int init(const traa_config *config);

  int set_event_handler(const traa_event_handler *handler);

  int enum_device_info(traa_device_type type, traa_device_info **infos, int *count);

  int free_device_info(traa_device_info infos[]);

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
  static int enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                     const unsigned int external_flags,
                                     traa_screen_source_info **infos, int *count);

  static int free_screen_source_info(traa_screen_source_info infos[], int count);

  static int create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                             int *data_size, traa_size *actual_size);

  static void free_snapshot(uint8_t *data);

  int start_screen_capture(const traa_screen_capture_config *config);
  int stop_screen_capture(const int64_t source_id);
#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

  // Camera capability query
  int get_camera_capability(const char *device_id, traa_video_capability **capabilities,
                            int *count);
  int free_camera_capability(traa_video_capability *capabilities);

  // Camera capture management
  int start_camera_capture(const traa_camera_config *config);
  int stop_camera_capture(const char *device_id);

private:
  // Ensure camera device_info is initialized (lazy init)
  base::device_info_impl *ensure_camera_device_info();

  // Camera device info (lazy init)
  std::unique_ptr<base::device_info_impl> camera_device_info_;

  // Frame callback adapter: bridges video_frame_callback → user C function pointer
  struct camera_capture_context {
    std::unique_ptr<base::video_capture_impl> capture;
    std::unique_ptr<base::video_frame_callback> adapter;
  };

  // Active camera captures, indexed by device_id
  std::unordered_map<std::string, camera_capture_context> camera_captures_;

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
  // Screen capture context: one per active screen/window capture session
  struct screen_capture_context {
    std::unique_ptr<base::desktop_capturer> capturer;
    std::unique_ptr<std::thread> capture_thread;
    std::atomic<bool> running{false};
    void (*on_video_frame)(const traa_userdata, const traa_video_frame *) = nullptr;
    traa_userdata userdata = nullptr;
    traa_size frame_size;

    screen_capture_context() = default;
    screen_capture_context(screen_capture_context &&) = default;
    screen_capture_context &operator=(screen_capture_context &&) = default;

    DISALLOW_COPY_AND_ASSIGN(screen_capture_context);
  };

  // Active screen captures, indexed by source_id
  std::unordered_map<int64_t, screen_capture_context> screen_captures_;
#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
};

} // namespace main
} // namespace traa

#endif // TRAA_MAIN_ENGINE_H_