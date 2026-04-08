#include "base/devices/camera/video_capture_defines.h"

#include <cassert>

#include <libyuv.h>

namespace traa {
namespace base {

size_t calc_buffer_size(video_type type, int32_t width, int32_t height) {
  assert(width >= 0);
  assert(height >= 0);
  switch (type) {
    case video_type::k_i420:
    case video_type::k_iyuv:
    case video_type::k_yv12:
    case video_type::k_nv12: {
      int half_width = (width + 1) >> 1;
      int half_height = (height + 1) >> 1;
      return width * height + half_width * half_height * 2;
    }
    case video_type::k_rgb565:
    case video_type::k_yuy2:
    case video_type::k_uyvy:
      return width * height * 2;
    case video_type::k_rgb24:
      return width * height * 3;
    case video_type::k_mjpeg:
    case video_type::k_unknown:
      return 0;
  }
  return 0;
}

uint32_t convert_video_type(video_type type) {
  switch (type) {
    case video_type::k_unknown:
      return libyuv::FOURCC_ANY;
    case video_type::k_i420:
      return libyuv::FOURCC_I420;
    case video_type::k_iyuv:
    case video_type::k_yv12:
      return libyuv::FOURCC_YV12;
    case video_type::k_rgb24:
      return libyuv::FOURCC_24BG;
    case video_type::k_rgb565:
      return libyuv::FOURCC_RGBP;
    case video_type::k_yuy2:
      return libyuv::FOURCC_YUY2;
    case video_type::k_uyvy:
      return libyuv::FOURCC_UYVY;
    case video_type::k_mjpeg:
      return libyuv::FOURCC_MJPG;
    case video_type::k_nv12:
      return libyuv::FOURCC_NV12;
    default:
      return libyuv::FOURCC_ANY;
  }
}

} // namespace base
} // namespace traa
