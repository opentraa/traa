#include "traa/traa.h"

#include "base/logger.h"
#include "base/thread/task_queue.h"
#include "main/engine.h"
#include "main/utils/obj_string.h"

#include <memory>

namespace {
// The main queue id.
static const traa::base::task_queue::task_queue_id_t g_main_queue_id = 0;

// The main queue name.
static const char *k_main_queue_name = "traa_main";

// The engine instance.
// The engine instance is created when traa_init is called and deleted when traa_release is called.
// The engine instance is a thread local variable to avoid the need for locking when accessing it,
// so do not use it outside of the main queue.
thread_local traa::main::engine *g_engine_instance = nullptr;
} // namespace

// TODO @sylar: replace this with api counters in the future.
#define TRAA_TEST_ENABLE_API_COUNTER 0
#if TRAA_TEST_ENABLE_API_COUNTER
static std::atomic<long> g_new_count(0);
static std::atomic<long> g_delete_count(0);
static std::atomic<long> g_traa_init_count(0);
static std::atomic<long> g_traa_release_count(0);
#endif // TRAA_UNIT_TEST

int traa_init(const traa_config *config) {
  if (config == nullptr) {
    LOG_ERROR("traa_config is null");
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (config->log_config.log_file != nullptr) {
    traa::base::logger::set_log_file(config->log_config.log_file, config->log_config.max_size,
                                     config->log_config.max_files);
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(config->log_config.level));
  }

  LOG_API_ARGS_1(traa::main::obj_string::to_string(config));

  // no need to lock here coz we have rw lock in task_queue_manager
  if (!traa::base::task_queue_manager::is_task_queue_exist(g_main_queue_id) &&
      !traa::base::task_queue_manager::create_queue(g_main_queue_id, k_main_queue_name, []() {
        if (g_engine_instance) {
          delete g_engine_instance;
          g_engine_instance = nullptr;
#if TRAA_TEST_ENABLE_API_COUNTER
          printf("delete engine instance: %ld\r\n", g_delete_count.fetch_add(1) + 1);
#endif // TRAA_UNIT_TEST
        }
      })) {
    LOG_FATAL("failed to create main queue");
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

#if TRAA_TEST_ENABLE_API_COUNTER
  printf("traa_init: %ld\r\n", g_traa_init_count.fetch_add(1) + 1);
#endif // TRAA_UNIT_TEST

  int ret = traa::base::task_queue_manager::post_task(g_main_queue_id, [&config]() {
              if (g_engine_instance == nullptr) {
                g_engine_instance = new traa::main::engine();
#if TRAA_TEST_ENABLE_API_COUNTER
                printf("new engine instance: %ld\r\n", g_new_count.fetch_add(1) + 1);
#endif // TRAA_UNIT_TEST
              }

              if (g_engine_instance == nullptr) {
                LOG_FATAL("failed to create engine instance");
                return static_cast<int>(traa_error::TRAA_ERROR_UNKNOWN);
              }

              return g_engine_instance->init(config);
            }).get(traa_error::TRAA_ERROR_UNKNOWN);

  if (ret != traa_error::TRAA_ERROR_NONE && ret != traa_error::TRAA_ERROR_ALREADY_INITIALIZED) {
    // to make sure that the main queue is released if the engine initialization failed.
    // so that we do not need to adjust the engine is exist or not in other places.
    traa::base::task_queue_manager::release_queue(g_main_queue_id);
  }

  return ret;
}

void traa_release() {
  LOG_API_ARGS_0();

#if TRAA_TEST_ENABLE_API_COUNTER
  printf("traa_release: %ld\r\n", g_traa_release_count.fetch_add(1) + 1);
#endif // TRAA_UNIT_TEST

  traa::base::task_queue_manager::shutdown();
}

int traa_set_event_handler(const traa_event_handler *handler) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(handler));

  if (handler == nullptr) {
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  return traa::base::task_queue_manager::post_task(
             g_main_queue_id,
             [&handler]() { return g_engine_instance->set_event_handler(handler); })
      .get(traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

void traa_set_log_level(traa_log_level level) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(level));

  traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(level));
}

int traa_set_log(const traa_log_config *config) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(config));

  if (config == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (config->log_file != nullptr) {
    // call set_level before set_log_file to ensure that no log messages written to the file or
    // stdout in case that user sets the log level to a higher level.
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(config->level));
    traa::base::logger::set_log_file(config->log_file, config->max_size, config->max_files);
  }

  return TRAA_ERROR_NONE;
}

int traa_enum_device_info(traa_device_type type, traa_device_info **infos, int *count) {
  LOG_API_ARGS_3(traa::main::obj_string::to_string(type), traa::main::obj_string::to_string(infos),
                 traa::main::obj_string::to_string(count));

  if (infos == nullptr && count == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  return traa::base::task_queue_manager::post_task(g_main_queue_id,
                                                   [type, infos, count]() {
                                                     return g_engine_instance->enum_device_info(
                                                         type, infos, count);
                                                   })
      .get(traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

int traa_free_device_info(traa_device_info infos[]) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(infos));

  if (infos == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  return traa::base::task_queue_manager::post_task(
             g_main_queue_id, [infos]() { return g_engine_instance->free_device_info(infos); })
      .get(traa_error::TRAA_ERROR_NOT_INITIALIZED);
}

#if !defined(__ANDROID__)
#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)
int traa_enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                 const unsigned int external_flags, traa_screen_source_info **infos,
                                 int *count) {
  LOG_API_ARGS_5(traa::main::obj_string::to_string(icon_size),
                 traa::main::obj_string::to_string(thumbnail_size), std::to_string(external_flags),
                 traa::main::obj_string::to_string(infos),
                 traa::main::obj_string::to_string(count));

  if (infos == nullptr && count == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  return traa::main::engine::enum_screen_source_info(icon_size, thumbnail_size, external_flags,
                                                     infos, count);
}

int traa_free_screen_source_info(traa_screen_source_info infos[], int count) {
  LOG_API_ARGS_2(traa::main::obj_string::to_string(infos), std::to_string(count));

  if (infos == nullptr) {
    return TRAA_ERROR_NONE;
  }

  return traa::main::engine::free_screen_source_info(infos, count);
}

int traa_create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                         int *data_size, traa_size *actual_size) {
  LOG_API_ARGS_5(source_id, traa::main::obj_string::to_string(snapshot_size),
                 traa::main::obj_string::to_string(data),
                 traa::main::obj_string::to_string(data_size),
                 traa::main::obj_string::to_string(actual_size));

  return traa::main::engine::create_snapshot(source_id, snapshot_size, data, data_size,
                                             actual_size);
}

void traa_free_snapshot(uint8_t *data) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(data));

  return traa::main::engine::free_snapshot(data);
}

#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE && (!defined(TARGET_OS_VISION)
       // || !TARGET_OS_VISION)) || __linux__
#endif