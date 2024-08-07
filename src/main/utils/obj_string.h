#ifndef TRAA_MAIN_UTILS_OBJ_STRING_H
#define TRAA_MAIN_UTILS_OBJ_STRING_H

#include <iomanip>
#include <sstream>
#include <string>

#include "base/disallow.h"

#include <traa/base.h>

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
   * @param log_config The TRAA log config.
   *
   * @return The string.
   */
  static std::string to_string(const traa_log_config *log_config) {
    std::stringstream ss;
    if (log_config == nullptr) {
      ss << "{null}";
    } else {
      ss << "{"
         << "\"log_file\": " << (log_config->log_file ? log_config->log_file : "null") << ", "
         << "\"max_size\": " << log_config->max_size << ", "
         << "\"max_files\": " << log_config->max_files << ", "
         << "\"level\": " << to_string(log_config->level) << "}";
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
  static std::string to_string(const traa_event_handler *event_handler) {
    std::stringstream ss;
    if (event_handler == nullptr) {
      ss << "{null}";
    } else {
      ss << "{"
         << "\"on_error\": "
         << number_to_hexstring(reinterpret_cast<std::uintptr_t>(event_handler->on_error)) << ", "
         << "\"on_device_event\": "
         << number_to_hexstring(reinterpret_cast<std::uintptr_t>(event_handler->on_device_event))
         << ", "
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

#endif // TRAA_MAIN_UTILS_OBJ_STRING_H