#ifndef TRAA_UNIT_TEST_UTILS_TEST_LOGGER_H_
#define TRAA_UNIT_TEST_UTILS_TEST_LOGGER_H_
#include "base/logger.h"

#define TEST_LOG_DEBUG(TXT) LOG(spdlog::level::debug, "[ut]: " #TXT)
#define TEST_LOG_INFO(TXT) LOG(spdlog::level::info, "[ut]: " #TXT)
#define TEST_LOG_WARN(TXT) LOG(spdlog::level::warn, "[ut]: " #TXT)
#define TEST_LOG_ERROR(TXT) LOG(spdlog::level::err, "[ut]: " #TXT)
#define TEST_LOG_FATAL(TXT) LOG(spdlog::level::critical, "[ut]: " #TXT)

#define TEST_LOG_DEBUG_N(FMT, ...) LOG(spdlog::level::debug "[ut]: " #FMT, __VA_ARGS__)
#define TEST_LOG_INFO_N(FMT, ...) LOG(spdlog::level::info "[ut]: " #FMT, __VA_ARGS__)
#define TEST_LOG_WARN_N(FMT, ...) LOG(spdlog::level::warn "[ut]: " #FMT, __VA_ARGS__)
#define TEST_LOG_ERROR_N(FMT, ...) LOG(spdlog::level::err "[ut]: " #FMT, __VA_ARGS__)
#define TEST_LOG_FATAL_N(FMT, ...) LOG(spdlog::level::critical "[ut]: " #FMT, __VA_ARGS__)

// DO NOT ALLOW TO USE LOG MACROS IN UNIT TESTS
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR
#undef LOG_FATAL

#endif // TRAA_UNIT_TEST_UTILS_TEST_LOGGER_H_