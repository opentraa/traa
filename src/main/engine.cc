#include "main/engine.h"

#include "base/devices/camera/device_info_impl.h"
#include "base/devices/camera/video_capture_impl.h"
#include "base/devices/screen/enumerator.h"
#include "base/logger.h"

#include "main/utils/obj_string.h"

namespace traa {
namespace base {
// Factory functions declared in platform-specific .cc (e.g., video_capture_factory_windows.cc).
device_info_impl *create_device_info();
video_capture_impl *create_video_capture(const char *device_id);
} // namespace base
} // namespace traa

namespace traa {
namespace main {

namespace {

// Bridges the internal video_frame_callback interface to the user's C function pointer.
// Called on the capture thread — not the main task queue — to avoid frame data copies.
class frame_callback_adapter : public base::video_frame_callback {
public:
  frame_callback_adapter(void (*callback)(const traa_userdata, const traa_video_frame *),
                         traa_userdata userdata)
      : callback_(callback), userdata_(userdata) {}

  void on_frame(const uint8_t *buffer, int32_t width, int32_t height, size_t length,
                int64_t timestamp_ms) override {
    traa_video_frame frame;
    frame.data = buffer;
    frame.data_length = static_cast<int32_t>(length);
    frame.width = width;
    frame.height = height;
    frame.format = TRAA_VIDEO_FRAME_FORMAT_I420;
    frame.timestamp_ms = timestamp_ms;
    callback_(userdata_, &frame);
  }

private:
  void (*callback_)(const traa_userdata, const traa_video_frame *);
  traa_userdata userdata_;
};

// Convert public traa_video_capability to internal video_capture_capability.
static base::video_capture_capability
to_internal_capability(const traa_video_capability &cap) {
  base::video_capture_capability result;
  result.width = cap.width;
  result.height = cap.height;
  result.max_fps = cap.max_fps;
  result.video_type = static_cast<base::video_type>(cap.format);
  result.interlaced = cap.interlaced;
  return result;
}

// Convert internal video_capture_capability to public traa_video_capability.
static traa_video_capability
to_public_capability(const base::video_capture_capability &cap) {
  traa_video_capability result;
  result.width = cap.width;
  result.height = cap.height;
  result.max_fps = cap.max_fps;
  result.format = static_cast<traa_video_frame_format>(cap.video_type);
  result.interlaced = cap.interlaced;
  return result;
}

} // namespace

engine::engine() { LOG_API_ARGS_0(); }

engine::~engine() {
  LOG_API_ARGS_0();

  // Stop all active camera captures
  for (auto &pair : camera_captures_) {
    pair.second.capture->stop_capture();
    pair.second.capture->deregister_capture_data_callback();
  }
  camera_captures_.clear();
  camera_device_info_.reset();
}

int engine::init(const traa_config *config) { return traa_error::TRAA_ERROR_NONE; }

int engine::set_event_handler(const traa_event_handler *handler) {
  return traa_error::TRAA_ERROR_NONE;
}

base::device_info_impl *engine::ensure_camera_device_info() {
  if (!camera_device_info_) {
    camera_device_info_.reset(base::create_device_info());
  }
  return camera_device_info_.get();
}

int engine::enum_device_info(traa_device_type type, traa_device_info **infos, int *count) {
  if (type == TRAA_DEVICE_TYPE_CAMERA) {
    auto *di = ensure_camera_device_info();
    if (!di) {
      if (count)
        *count = 0;
      if (infos)
        *infos = nullptr;
      return TRAA_ERROR_NONE;
    }
    uint32_t num = di->number_of_devices();
    if (num == 0) {
      if (count)
        *count = 0;
      if (infos)
        *infos = nullptr;
      return TRAA_ERROR_NONE;
    }
    auto *arr = new traa_device_info[num]();
    for (uint32_t i = 0; i < num; ++i) {
      di->get_device_name(i, arr[i].name, TRAA_MAX_DEVICE_NAME_LENGTH, arr[i].id,
                          TRAA_MAX_DEVICE_ID_LENGTH, nullptr, 0);
      arr[i].type = TRAA_DEVICE_TYPE_CAMERA;
    }
    if (infos)
      *infos = arr;
    if (count)
      *count = static_cast<int>(num);
    return TRAA_ERROR_NONE;
  }
  return traa_error::TRAA_ERROR_NONE;
}

int engine::free_device_info(traa_device_info infos[]) { return traa_error::TRAA_ERROR_NONE; }

int engine::get_camera_capability(const char *device_id, traa_video_capability **capabilities,
                                  int *count) {
  auto *di = ensure_camera_device_info();
  if (!di) {
    return TRAA_ERROR_NOT_FOUND;
  }
  int32_t num = di->number_of_capabilities(device_id);
  if (num <= 0) {
    return TRAA_ERROR_NOT_FOUND;
  }
  auto *arr = new traa_video_capability[num]();
  for (int32_t i = 0; i < num; ++i) {
    base::video_capture_capability internal_cap;
    di->get_capability(device_id, static_cast<uint32_t>(i), internal_cap);
    arr[i] = to_public_capability(internal_cap);
  }
  if (capabilities)
    *capabilities = arr;
  if (count)
    *count = num;
  return TRAA_ERROR_NONE;
}

int engine::free_camera_capability(traa_video_capability *capabilities) {
  delete[] capabilities;
  return TRAA_ERROR_NONE;
}

int engine::start_camera_capture(const traa_camera_config *config) {
  std::string device_id(config->device_id);

  // Check for duplicate
  if (camera_captures_.find(device_id) != camera_captures_.end()) {
    return TRAA_ERROR_ALREADY_EXISTS;
  }

  // Create capture instance
  std::unique_ptr<base::video_capture_impl> capture(
      base::create_video_capture(device_id.c_str()));
  if (!capture) {
    return TRAA_ERROR_UNKNOWN;
  }

  // Create adapter and register callback
  auto adapter =
      std::make_unique<frame_callback_adapter>(config->on_video_frame, config->userdata);
  capture->register_capture_data_callback(adapter.get());

  // Convert capability and start capture
  auto internal_cap = to_internal_capability(config->capability);
  int ret = capture->start_capture(internal_cap);
  if (ret != 0) {
    capture->deregister_capture_data_callback();
    return TRAA_ERROR_UNKNOWN;
  }

  // Store context
  camera_capture_context ctx;
  ctx.capture = std::move(capture);
  ctx.adapter = std::move(adapter);
  camera_captures_[device_id] = std::move(ctx);

  return TRAA_ERROR_NONE;
}

int engine::stop_camera_capture(const char *device_id) {
  std::string id(device_id);
  auto it = camera_captures_.find(id);
  if (it == camera_captures_.end()) {
    return TRAA_ERROR_NOT_FOUND;
  }
  it->second.capture->stop_capture();
  it->second.capture->deregister_capture_data_callback();
  camera_captures_.erase(it);
  return TRAA_ERROR_NONE;
}

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
int engine::enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                    const unsigned int external_flags,
                                    traa_screen_source_info **infos, int *count) {
  return base::screen_source_info_enumerator::enum_screen_source_info(icon_size, thumbnail_size,
                                                                      external_flags, infos, count);
}

int engine::free_screen_source_info(traa_screen_source_info infos[], int count) {
  return base::screen_source_info_enumerator::free_screen_source_info(infos, count);
}

int engine::create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                            int *data_size, traa_size *actual_size) {
  return base::screen_source_info_enumerator::create_snapshot(source_id, snapshot_size, data,
                                                              data_size, actual_size);
}

void engine::free_snapshot(uint8_t *data) {
  return base::screen_source_info_enumerator::free_snapshot(data);
}

#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

} // namespace main
} // namespace traa
