#ifndef TRAA_MAIN_UTILS_OBJ_STRING_H_
#define TRAA_MAIN_UTILS_OBJ_STRING_H_

#include <iomanip>
#include <sstream>
#include <string>

#include "base/disallow.h"

#include <traa/base.h>

#define OBJ_STRING_STREAM_DEFINE std::stringstream ss
#define OBJ_STRING_SEP ss << ", "
#define OBJ_STRING_OBJ_BEGIN ss << "{"
#define OBJ_STRING_OBJ_END ss << "}"
#define OBJ_STRING_POINTER_NULL ss << "null"
#define OBJ_STRING_PROPERTY(OBJ, PROP) ss << "\"" #PROP "\": " << (OBJ).PROP
#define OBJ_STRING_PROPERTY_STR(OBJ, PROP)                                                         \
  ss << "\"" #PROP "\": \"" << ((OBJ).PROP ? (OBJ).PROP : "empty") << "\""
#define OBJ_STRING_PROPERTY_POINTER(OBJ, PROP)                                                     \
  ss << "\"" #PROP "\": " << number_to_hexstring(reinterpret_cast<std::uintptr_t>((OBJ).PROP))
#define OBJ_STRING_PROPERTY_TRAA_OBJ(OBJ, PROP) ss << "\"" #PROP "\": " << to_string((OBJ).PROP)
#define OBJ_STRING_STREAM_RETURN return ss.str()

namespace traa {
namespace main {
/**
 * @brief The `obj_string` class provides utility functions for converting objects to strings.
 *
 * The `obj_string` class contains static member functions that can be used to convert various types
 * of objects to strings. It provides functions for converting numbers to hex strings, TRAA contexts
 * to strings, TRAA log levels to strings, TRAA log configs to strings, and TRAA event handlers to
 * strings.
 */
class obj_string {
  DISALLOW_IMPLICIT_CONSTRUCTORS(obj_string);

public:
  /**
   * @brief Convert a number to a hex string.
   *
   * This function converts a number to a hex string.
   *
   * @tparam T The type of the number.
   * @param i The number.
   *
   * @return The hex string.
   */
  template <typename T> static std::string number_to_hexstring(T i) {
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return stream.str();
  }

  /**
   * @brief Convert a TRAA context to a string.
   *
   * This function converts a TRAA context to a string.
   *
   * @param context The TRAA context.
   *
   * @return The string.
   */
  static std::string to_string(const void *context) {
    return number_to_hexstring(reinterpret_cast<std::uintptr_t>(context));
  }

  /**
   * @brief Convert a TRAA size to a string.
   *
   * This function converts a TRAA size to a string.
   *
   * @param size The TRAA size.
   *
   * @return The string.
   */
  static std::string to_string(const traa_size &size) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    OBJ_STRING_PROPERTY(size, width);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(size, height);

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  /**
   * @brief Convert a TRAA point to a string.
   *
   * This function converts a TRAA point to a string.
   *
   * @param point The TRAA point.
   *
   * @return The string.
   */
  static std::string to_string(const traa_point &point) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    OBJ_STRING_PROPERTY(point, x);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(point, y);

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  /**
   * @brief Convert a TRAA rect to a string.
   *
   * This function converts a TRAA rect to a string.
   *
   * @param rect The TRAA rect.
   *
   * @return The string.
   */
  static std::string to_string(const traa_rect &rect) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    OBJ_STRING_PROPERTY(rect, left);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, top);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, right);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, bottom);

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  /**
   * @brief Convert a TRAA device type to a string.
   *
   * This function converts a TRAA device type to a string.
   *
   * @param type The TRAA device type.
   *
   * @return The string.
   */
  static std::string to_string(traa_device_type type) {
    switch (type) {
    case TRAA_DEVICE_TYPE_UNKNOWN:
      return "unknown";
    case TRAA_DEVICE_TYPE_CAMERA:
      return "camera";
    case TRAA_DEVICE_TYPE_MICROPHONE:
      return "microphone";
    case TRAA_DEVICE_TYPE_SPEAKER:
      return "speaker";
    case TRAA_DEVICE_TYPE_MEDIA_FILE:
      return "media_file";
    default:
      return "unknown";
    }
  }

  /**
   * @brief Convert a TRAA log level to a string.
   *
   * This function converts a TRAA log level to a string.
   *
   * @param level The TRAA log level.
   *
   * @return The string.
   */
  static std::string to_string(traa_log_level level) {
    switch (level) {
    case TRAA_LOG_LEVEL_TRACE:
      return "trace";
    case TRAA_LOG_LEVEL_DEBUG:
      return "debug";
    case TRAA_LOG_LEVEL_INFO:
      return "info";
    case TRAA_LOG_LEVEL_WARN:
      return "warn";
    case TRAA_LOG_LEVEL_ERROR:
      return "error";
    default:
      return "unknown";
    }
  }

  /**
   * @brief Convert a TRAA log config to a string.
   *
   * This function converts a TRAA log config to a string.
   *
   * @param config The TRAA log config.
   *
   * @return The string.
   */
  static std::string to_string(const traa_log_config *config) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (config == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY_STR(*config, log_file);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*config, max_size);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*config, max_files);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*config, level);
    }

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_log_config &config) { return to_string(&config); }

  /**
   * @brief Convert a TRAA event handler to a string.
   *
   * This function converts a TRAA event handler to a string.
   *
   * @param event_handler The TRAA event handler.
   *
   * @return The string.
   */
  static std::string to_string(const traa_event_handler *handler) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (handler == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY_POINTER(*handler, on_error);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_POINTER(*handler, on_device_event);
    }

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_event_handler &handler) { return to_string(&handler); }

  /**
   * @brief Convert a TRAA config to a string.
   *
   * This function converts a TRAA config to a string.
   *
   * @param config The TRAA config.
   *
   * @return The string.
   */
  static std::string to_string(const traa_config *config) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (config == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, userdata);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, log_config);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, event_handler);
    }

    OBJ_STRING_STREAM_RETURN;
  }

  /**
   * @brief Convert a TRAA video frame format to a string.
   *
   * This function converts a TRAA video frame format to a string.
   *
   * @param format The TRAA video frame format.
   *
   * @return The string.
   */
  static std::string to_string(traa_video_frame_format format) {
    switch (format) {
    case TRAA_VIDEO_FRAME_FORMAT_UNKNOWN:
      return "unknown";
    case TRAA_VIDEO_FRAME_FORMAT_I420:
      return "i420";
    case TRAA_VIDEO_FRAME_FORMAT_IYUV:
      return "iyuv";
    case TRAA_VIDEO_FRAME_FORMAT_RGB24:
      return "rgb24";
    case TRAA_VIDEO_FRAME_FORMAT_YUY2:
      return "yuy2";
    case TRAA_VIDEO_FRAME_FORMAT_YV12:
      return "yv12";
    case TRAA_VIDEO_FRAME_FORMAT_RGB565:
      return "rgb565";
    case TRAA_VIDEO_FRAME_FORMAT_NV12:
      return "nv12";
    case TRAA_VIDEO_FRAME_FORMAT_UYVY:
      return "uyvy";
    case TRAA_VIDEO_FRAME_FORMAT_MJPEG:
      return "mjpeg";
    case TRAA_VIDEO_FRAME_FORMAT_BGRA:
      return "bgra";
    default:
      return "unknown";
    }
  }

  /**
   * @brief Convert a TRAA video capability to a string.
   *
   * This function converts a TRAA video capability to a string.
   *
   * @param cap The TRAA video capability.
   *
   * @return The string.
   */
  static std::string to_string(const traa_video_capability *cap) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (cap == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY(*cap, width);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*cap, height);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*cap, max_fps);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*cap, format);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*cap, interlaced);
    }

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_video_capability &cap) { return to_string(&cap); }

  /**
   * @brief Convert a TRAA video frame to a string.
   *
   * This function converts a TRAA video frame to a string.
   *
   * @param frame The TRAA video frame.
   *
   * @return The string.
   */
  static std::string to_string(const traa_video_frame *frame) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (frame == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY_POINTER(*frame, data);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*frame, data_length);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*frame, width);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*frame, height);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*frame, format);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY(*frame, timestamp_ms);
    }

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_video_frame &frame) { return to_string(&frame); }

  /**
   * @brief Convert a TRAA camera config to a string.
   *
   * This function converts a TRAA camera config to a string.
   *
   * @param config The TRAA camera config.
   *
   * @return The string.
   */
  static std::string to_string(const traa_camera_config *config) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (config == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY_STR(*config, device_id);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, capability);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_POINTER(*config, on_video_frame);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, userdata);
    }

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_camera_config &config) { return to_string(&config); }

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)
  /**
   * @brief Convert a TRAA screen capture config to a string.
   *
   * This function converts a TRAA screen capture config to a string.
   *
   * @param config The TRAA screen capture config.
   *
   * @return The string.
   */
  static std::string to_string(const traa_screen_capture_config *config) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEGIN;

    if (config == nullptr) {
      OBJ_STRING_POINTER_NULL;
    } else {
      OBJ_STRING_PROPERTY(*config, source_id);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, frame_size);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_POINTER(*config, on_video_frame);
      OBJ_STRING_SEP;

      OBJ_STRING_PROPERTY_TRAA_OBJ(*config, userdata);
    }

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(const traa_screen_capture_config &config) {
    return to_string(&config);
  }
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE && (!TARGET_OS_VISION ||
       // !TARGET_OS_VISION)) || __linux__
};

} // namespace main
} // namespace traa

#endif // TRAA_MAIN_UTILS_OBJ_STRING_H_