#include "main/engine.h"

#include "base/devices/camera/device_info_impl.h"
#include "base/devices/camera/video_capture_impl.h"
#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/enumerator.h"
#include "base/logger.h"

#include "main/utils/obj_string.h"

#include "libyuv/scale_argb.h"

#include <chrono>
#include <cstring>
#include <tuple>
#include <vector>

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

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

// Adapts desktop_capturer::capture_callback to the user's on_video_frame C function pointer.
// Handles BGRA frame wrapping and optional scaling via libyuv::ARGBScale.
class screen_capture_callback : public base::desktop_capturer::capture_callback {
public:
  screen_capture_callback(
      void (*on_video_frame)(const traa_userdata, const traa_video_frame *),
      traa_userdata userdata, traa_size frame_size)
      : on_video_frame_(on_video_frame), userdata_(userdata), frame_size_(frame_size) {}

  void on_capture_result(base::desktop_capturer::capture_result result,
                         std::unique_ptr<base::desktop_frame> frame) override {
    if (result != base::desktop_capturer::capture_result::success || !frame) {
      return;
    }

    const int src_width = frame->size().width();
    const int src_height = frame->size().height();
    if (src_width <= 0 || src_height <= 0) {
      return;
    }

    const uint8_t *src_data = frame->data();
    const int src_stride = frame->stride();

    const bool need_scale = frame_size_.width > 0 && frame_size_.height > 0 &&
                            (frame_size_.width != src_width || frame_size_.height != src_height);

    traa_video_frame video_frame;
    video_frame.format = TRAA_VIDEO_FRAME_FORMAT_BGRA;

    if (need_scale) {
      const int dst_width = frame_size_.width;
      const int dst_height = frame_size_.height;
      const int dst_stride = dst_width * 4;
      const size_t dst_size = static_cast<size_t>(dst_stride) * dst_height;

      scale_buffer_.resize(dst_size);

      int ret = libyuv::ARGBScale(src_data, src_stride, src_width, src_height,
                                   scale_buffer_.data(), dst_stride, dst_width, dst_height,
                                   libyuv::kFilterBilinear);
      if (ret != 0) {
        LOG_WARN("ARGBScale failed with error {}", ret);
        return;
      }

      video_frame.data = scale_buffer_.data();
      video_frame.data_length = static_cast<int32_t>(dst_size);
      video_frame.width = dst_width;
      video_frame.height = dst_height;
    } else {
      const int packed_stride = src_width * 4;
      if (src_stride != packed_stride) {
        // desktop_frame has row padding — strip it to produce tightly-packed BGRA
        const size_t packed_size = static_cast<size_t>(packed_stride) * src_height;
        scale_buffer_.resize(packed_size);
        for (int row = 0; row < src_height; ++row) {
          std::memcpy(scale_buffer_.data() + row * packed_stride,
                      src_data + row * src_stride, packed_stride);
        }
        video_frame.data = scale_buffer_.data();
        video_frame.data_length = static_cast<int32_t>(packed_size);
      } else {
        video_frame.data = src_data;
        video_frame.data_length = src_width * src_height * 4;
      }
      video_frame.width = src_width;
      video_frame.height = src_height;
    }

    video_frame.timestamp_ms = frame->capture_time_ms();

    on_video_frame_(userdata_, &video_frame);
  }

private:
  void (*on_video_frame_)(const traa_userdata, const traa_video_frame *);
  traa_userdata userdata_;
  traa_size frame_size_;
  std::vector<uint8_t> scale_buffer_;
};

#endif // desktop platform guard

} // namespace

engine::engine() { LOG_API_ARGS_0(); }

engine::~engine() {
  LOG_API_ARGS_0();

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
  // Stop all active screen captures
  for (auto &pair : screen_captures_) {
    pair.second.running.store(false);
    if (pair.second.capture_thread && pair.second.capture_thread->joinable()) {
      pair.second.capture_thread->join();
    }
  }
  screen_captures_.clear();
#endif

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

int engine::start_screen_capture(const traa_screen_capture_config *config) {
  const int64_t source_id = config->source_id;

  // Check for duplicate
  if (screen_captures_.find(source_id) != screen_captures_.end()) {
    LOG_WARN("screen capture for source_id {} already exists", source_id);
    return TRAA_ERROR_ALREADY_EXISTS;
  }

  // Enumerate sources to determine if this is a window or screen
  traa_screen_source_info *infos = nullptr;
  int count = 0;
  int ret = enum_screen_source_info(traa_size(), traa_size(),
                                    TRAA_SCREEN_SOURCE_FLAG_NONE, &infos, &count);
  if (ret != TRAA_ERROR_NONE || infos == nullptr || count <= 0) {
    LOG_ERROR("failed to enumerate screen source info, error: {}", ret);
    return TRAA_ERROR_NOT_FOUND;
  }

  bool found = false;
  bool is_window = false;
  for (int i = 0; i < count; ++i) {
    if (infos[i].id == source_id) {
      found = true;
      is_window = infos[i].is_window;
      break;
    }
  }

  free_screen_source_info(infos, count);

  if (!found) {
    LOG_ERROR("source_id {} not found in enumerated sources", source_id);
    return TRAA_ERROR_NOT_FOUND;
  }

  // Create the appropriate capturer
  auto options = base::desktop_capture_options::create_default();
  std::unique_ptr<base::desktop_capturer> capturer;
  if (is_window) {
    capturer = base::desktop_capturer::create_window_capturer(options);
  } else {
    capturer = base::desktop_capturer::create_screen_capturer(options);
  }

  if (!capturer) {
    LOG_ERROR("failed to create {} capturer for source_id {}",
              is_window ? "window" : "screen", source_id);
    return TRAA_ERROR_UNKNOWN;
  }

  // Create callback adapter
  auto callback = std::make_unique<screen_capture_callback>(
      config->on_video_frame, config->userdata, config->frame_size);

  // Start the capturer with the callback (raw pointer — callback must outlive capturer)
  capturer->start(callback.get());

  // Select the source
  if (!capturer->select_source(static_cast<base::desktop_capturer::source_id_t>(source_id))) {
    LOG_ERROR("failed to select source_id {}", source_id);
    return TRAA_ERROR_NOT_FOUND;
  }

  // Build the context and start the capture thread
  // Use emplace to construct in-place since screen_capture_context contains std::atomic
  auto [it, inserted] = screen_captures_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(source_id),
      std::forward_as_tuple());

  auto &stored_ctx = it->second;
  stored_ctx.capturer = std::move(capturer);
  stored_ctx.on_video_frame = config->on_video_frame;
  stored_ctx.userdata = config->userdata;
  stored_ctx.frame_size = config->frame_size;
  stored_ctx.running.store(true);

  // Move callback ownership into a shared_ptr so the thread can hold a reference
  auto shared_callback = std::shared_ptr<screen_capture_callback>(callback.release());

  base::desktop_capturer *capturer_ptr = stored_ctx.capturer.get();
  std::atomic<bool> *running_ptr = &stored_ctx.running;

  stored_ctx.capture_thread = std::make_unique<std::thread>(
      [capturer_ptr, running_ptr, shared_callback]() {
        constexpr auto frame_interval = std::chrono::milliseconds(33); // ~30 fps
        while (running_ptr->load()) {
          capturer_ptr->capture_frame();
          std::this_thread::sleep_for(frame_interval);
        }
      });

  LOG_INFO("screen capture started for source_id {}", source_id);
  return TRAA_ERROR_NONE;
}

int engine::stop_screen_capture(const int64_t source_id) {
  auto it = screen_captures_.find(source_id);
  if (it == screen_captures_.end()) {
    LOG_WARN("no active screen capture for source_id {}", source_id);
    return TRAA_ERROR_NOT_FOUND;
  }

  it->second.running.store(false);
  if (it->second.capture_thread && it->second.capture_thread->joinable()) {
    it->second.capture_thread->join();
  }

  screen_captures_.erase(it);

  LOG_INFO("screen capture stopped for source_id {}", source_id);
  return TRAA_ERROR_NONE;
}

#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

} // namespace main
} // namespace traa
