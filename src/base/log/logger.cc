#include "base/log/logger.h"
#include "base/folder/folder.h"
#include "base/platform.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#if defined(TRAA_OS_LINUX_ANDROID)
#include <spdlog/sinks/android_sink.h>
#endif // defined(TRAA_OS_LINUX_ANDROID)

#include <string>
#include <vector>

#if defined(TRAA_OS_WINDOWS)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace traa {
namespace base {

void logger::set_log_file(const std::string &filename, int max_size, int max_files) {
  std::string tmp_filename = filename;
  if (tmp_filename.empty()) {
    tmp_filename = folder::get_config_folder();
  }

  // If the filename contains a file extension means it is a file, not a folder.
  // So, remove the file name and use the folder.
  // Should we need to check the filename is a directory or not with native API?
  auto file_extension = folder::get_file_extension(tmp_filename);
  if (!file_extension.empty()) {
    tmp_filename = folder::get_directory(tmp_filename);
  }

  folder::append_filename(tmp_filename, "traa.log");

  std::vector<spdlog::sink_ptr> sinks;

  // Add the stdout sink.
#if defined(TRAA_OS_LINUX_ANDROID)
  sinks.emplace_back(std::make_shared<spdlog::sinks::android_sink_mt>());
#else
  sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
#endif

  // Add rotating file sink.
  sinks.emplace_back(
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>(tmp_filename, max_size, max_files));

  // Get the process id to make the logger name unique.
#if defined(TRAA_OS_WINDOWS)
  auto pid = _getpid();
#else
  auto pid = getpid();
#endif

  // Make and set the default logger.
  auto logger =
      std::make_shared<spdlog::logger>("traa-" + std::to_string(pid), sinks.begin(), sinks.end());
  logger->flush_on(spdlog::level::debug);
  logger->set_level(spdlog::get_level());
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%P][%t][%l][%s:%#] %v");

  spdlog::set_default_logger(logger);

  // TODO @sylar: find out why we call flush_every will cause the crash when app exit, the
  // periodic_worker is still running but the object is destroyed.
  //
  // DO NOT CALL flush_every unless all logger operations is thread-safe, from the spdlog
  // documentation.
  // spdlog::flush_every(std::chrono::seconds(2));

  LOG_INFO("initialize logger to {}, max file size {} count {}", tmp_filename, max_size, max_files);
}

void logger::set_level(spdlog::level::level_enum level) { spdlog::set_level(level); }

spdlog::level::level_enum logger::get_level() { return spdlog::get_level(); }

} // namespace base
} // namespace traa