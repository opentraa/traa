#ifndef TRAA_MAIN_UTILS_OBJ_STRING_H_
#define TRAA_MAIN_UTILS_OBJ_STRING_H_

#include <iomanip>
#include <sstream>
#include <string>

#include "base/disallow.h"

#include <traa/base.h>

#define OBJ_STRING_STREAM_DEFINE std::stringstream ss;
#define OBJ_STRING_SEP ss << ", "
#define OBJ_STRING_OBJ_BEING ss << "{"
#define OBJ_STRING_OBJ_END ss << "}"
#define OBJ_STRING_PROPERTY(OBJ, PROP) ss << "\"" #PROP "\": " << OBJ.PROP
#define OBJ_STRING_STREAM_RETURN return ss.str();

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

  static std::string to_string(int i) { return std::to_string(i); }

  static std::string to_string(traa_size size) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEING;

    OBJ_STRING_PROPERTY(size, width);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(size, height);

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(traa_point point) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEING;

    OBJ_STRING_PROPERTY(point, x);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(point, y);

    OBJ_STRING_OBJ_END;

    OBJ_STRING_STREAM_RETURN;
  }

  static std::string to_string(traa_rect rect) {
    OBJ_STRING_STREAM_DEFINE;

    OBJ_STRING_OBJ_BEING;

    OBJ_STRING_PROPERTY(rect, x);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, y);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, width);

    OBJ_STRING_SEP;
    OBJ_STRING_PROPERTY(rect, height);

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
    std::stringstream ss;
    if (config == nullptr) {
      ss << "{null}";
    } else {
      ss << "{"
         << "\"log_file\": " << (config->log_file ? config->log_file : "null") << ", "
         << "\"max_size\": " << config->max_size << ", "
         << "\"max_files\": " << config->max_files << ", "
         << "\"level\": " << to_string(config->level) << "}";
    }

    return ss.str();
  }

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
    std::stringstream ss;
    if (handler == nullptr) {
      ss << "{null}";
    } else {
      ss << "{"
         << "\"on_error\": "
         << number_to_hexstring(reinterpret_cast<std::uintptr_t>(handler->on_error)) << ", "
         << "\"on_device_event\": "
         << number_to_hexstring(reinterpret_cast<std::uintptr_t>(handler->on_device_event)) << ", "
         << "}";
    }

    return ss.str();
  }

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
    std::stringstream ss;
    if (config == nullptr) {
      ss << "{null}";
    } else {
      ss << "{"
         << "\"userdata\": " << to_string(config->userdata) << ", "
         << "\"log_config\": " << to_string(&config->log_config) << ", "
         << "\"event_handler\": " << to_string(&config->event_handler) << "}";
    }

    return ss.str();
  }
};
} // namespace main
} // namespace traa

#endif // TRAA_MAIN_UTILS_OBJ_STRING_H_