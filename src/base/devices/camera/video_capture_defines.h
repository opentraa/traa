#ifndef TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_DEFINES_H_
#define TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_DEFINES_H_

#include <cstddef>
#include <cstdint>

namespace traa {
namespace base {

enum class video_type {
  k_unknown = 0,
  k_i420,
  k_iyuv,
  k_rgb24,
  k_yuy2,
  k_yv12,
  k_rgb565,
  k_nv12,
  k_uyvy,
  k_mjpeg,
};

struct video_capture_capability {
  int32_t width = 0;
  int32_t height = 0;
  int32_t max_fps = 0;
  video_type video_type = video_type::k_unknown;
  bool interlaced = false;

  bool operator==(const video_capture_capability &other) const {
    return width == other.width && height == other.height && max_fps == other.max_fps &&
           video_type == other.video_type && interlaced == other.interlaced;
  }

  bool operator!=(const video_capture_capability &other) const { return !operator==(other); }
};

enum {
  k_video_capture_unique_name_length = 1024,
  k_video_capture_device_name_length = 256,
  k_video_capture_product_id_length = 128,
};

// Calculate the required buffer size for a given video type and resolution.
size_t calc_buffer_size(video_type type, int32_t width, int32_t height);

// Convert video_type to libyuv FourCC value.
uint32_t convert_video_type(video_type type);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_VIDEO_CAPTURE_DEFINES_H_
