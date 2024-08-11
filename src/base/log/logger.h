#ifndef TRAA_BASE_LOG_LOGGER_H_
#define TRAA_BASE_LOG_LOGGER_H_

#include "base/disallow.h"

#include <spdlog/spdlog.h>

#define LOG(LEVEL, ...)                                                                            \
  spdlog::default_logger()->log(spdlog::source_loc(__FILE__, __LINE__, nullptr),                   \
                                static_cast<spdlog::level::level_enum>(LEVEL), __VA_ARGS__)

#define LOG_DEBUG(...) LOG(spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...) LOG(spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...) LOG(spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) LOG(spdlog::level::err, __VA_ARGS__)
#define LOG_FATAL(...) LOG(spdlog::level::critical, __VA_ARGS__)

#if defined(_MSC_VER)
// #define FUNCTION_MACRO __FUNCSIG__
#define FUNCTION_MACRO __FUNCTION__
#else
// #define FUNCTION_MACRO __PRETTY_FUNCTION__
#define FUNCTION_MACRO __FUNCTION__
#endif

#define LOG_API(FORMAT, ...) LOG_INFO("call {}(" #FORMAT ")", FUNCTION_MACRO, __VA_ARGS__)
#define LOG_API_NO_ARGS() LOG_INFO("call {}()", FUNCTION_MACRO)
#define LOG_API_SCOPE_ENTER LOG_INFO("{} enter", FUNCTION_MACRO);
#define LOG_API_SCOPE_EXIT LOG_INFO("{} exit", FUNCTION_MACRO);
#define LOG_API_ONE_ARG(ARG) LOG_INFO("call {}({})", FUNCTION_MACRO, ARG)
#define LOG_API_TWO_ARGS(ARG1, ARG2) LOG_INFO("call {}({}, {})", FUNCTION_MACRO, ARG1, ARG2)
#define LOG_API_THREE_ARGS(ARG1, ARG2, ARG3)                                                       \
  LOG_INFO("call {}({}, {}, {})", FUNCTION_MACRO, ARG1, ARG2, ARG3)
#define LOG_API_FOUR_ARGS(ARG1, ARG2, ARG3, ARG4)                                                  \
  LOG_INFO("call {}({}, {}, {}, {})", FUNCTION_MACRO, ARG1, ARG2, ARG3, ARG4)
#define LOG_API_FIVE_ARGS(ARG1, ARG2, ARG3, ARG4, ARG5)                                            \
  LOG_INFO("call {}({}, {}, {}, {}, {})", FUNCTION_MACRO, ARG1, ARG2, ARG3, ARG4, ARG5)
#define LOG_API_SIX_ARGS(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)                                       \
  LOG_INFO("call {}({}, {}, {}, {}, {}, {})", FUNCTION_MACRO, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)
#define LOG_API_SEVEN_ARGS(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)                               \
  LOG_INFO("call {}({}, {}, {}, {}, {}, {}, {})", FUNCTION_MACRO, ARG1, ARG2, ARG3, ARG4, ARG5,    \
           ARG6, ARG7)

namespace traa {
namespace base {
/**
 * @brief The logger class provides functionality for logging messages to a file.
 */
class logger {
  DISALLOW_IMPLICIT_CONSTRUCTORS(logger);

public:
  /**
   * @brief Set the file properties of the logger.
   * @param filename The name of the log file.
   * @param max_size The maximum size of the log file.
   * @param max_files The maximum number of log files.
   */
  static void set_log_file(const std::string &filename, int max_size = 1024 * 1024 * 2 /*2MB*/,
                           int max_files = 3);

  /**
   * @brief Set the level of the logger.
   * @param level The level of the logger.
   */
  static void set_level(spdlog::level::level_enum level);

  /**
   * @brief Get the level of the logger.
   * @return The level of the logger.
   */
  static spdlog::level::level_enum get_level();
};
} // namespace base
} // namespace traa

#endif // TRAA_BASE_LOG_LOGGER_H_