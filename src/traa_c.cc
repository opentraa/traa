#include "traa/traa.h"

#include "base/log/logger.h"
#include "base/thread/task_queue.h"
#include "main/engine.h"
#include "utils/obj_string.h"

#include <memory>
#include <mutex>

namespace {
static std::mutex g_main_queue_mutex;
static const traa::base::task_queue::task_queue_id g_main_queue_id = 0;

// TODO @sylar: do we need to store this in the tls of main queue?
static traa::main::engine *g_engine_instance = nullptr;
} // namespace

int traa_init(const traa_config *config) {
  if (config == nullptr) {
    LOG_ERROR("traa_config is null");
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (config->log_config.log_file != nullptr) {
    traa::base::logger::set_log_file(config->log_config.log_file, config->log_config.max_size,
                                     config->log_config.max_files);
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(config->log_config.level));
  }

  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(config));

  std::lock_guard<std::mutex> lock(g_main_queue_mutex);
  auto main_queue = traa::base::task_queue_manager::get_task_queue(g_main_queue_id);
  if (!main_queue) {
    traa::base::task_queue_manager::create_queue(g_main_queue_id, "traa_main");
  }

  return traa::base::task_queue_manager::post_task(
             g_main_queue_id,
             [&config]() {
               if (g_engine_instance == nullptr) {
                 g_engine_instance = new traa::main::engine();
               }

               if (g_engine_instance == nullptr) {
                 LOG_FATAL("failed to create engine instance");
                 return static_cast<int>(traa_error::TRAA_ERROR_UNKNOWN);
               }

               return g_engine_instance->init(config);
             })
      .get();
}

void traa_release() {
  LOG_API_NO_ARGS();

  traa::base::task_queue_manager::post_task(g_main_queue_id, []() {
    if (g_engine_instance != nullptr) {
      delete g_engine_instance;
      g_engine_instance = nullptr;
    }
  }).wait();

  traa::base::task_queue_manager::shutdown();
}

void traa_set_log_level(traa_log_level level) {
  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(level));

  traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(level));
}

int traa_set_log(const traa_log_config *log_config) {
  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(log_config));

  if (log_config == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (log_config->log_file != nullptr) {
    // call set_level before set_log_file to ensure that no log messages written to the file or
    // stdout in case that user sets the log level to a higher level.
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(log_config->level));
    traa::base::logger::set_log_file(log_config->log_file, log_config->max_size,
                                     log_config->max_files);
  }

  return TRAA_ERROR_NONE;
}