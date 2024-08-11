#include "main/engine.h"

#include "base/log/logger.h"
#include "main/utils/obj_string.h"

namespace traa {
namespace main {

engine::engine() { LOG_API_ARGS_0(); }

engine::~engine() { LOG_API_ARGS_0(); }

int engine::init(const traa_config *config) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(config));

  return traa_error::TRAA_ERROR_NONE;
}

int engine::set_event_handler(const traa_event_handler *handler) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(handler));

  return traa_error::TRAA_ERROR_NONE;
}

int engine::enum_device_info(traa_device_type type, traa_device_info **infos, int *count) {
  LOG_API_ARGS_3(traa::main::obj_string::to_string(type), traa::main::obj_string::to_string(infos),
                 traa::main::obj_string::to_string(count));

  return traa_error::TRAA_ERROR_NONE;
}

int engine::free_device_info(traa_device_info infos[]) {
  LOG_API_ARGS_1(traa::main::obj_string::to_string(infos));

  return traa_error::TRAA_ERROR_NONE;
}

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
int engine::enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                    traa_screen_source_info **infos, int *count) {
  LOG_API_ARGS_4(traa::main::obj_string::to_string(icon_size),
                 traa::main::obj_string::to_string(thumbnail_size),
                 traa::main::obj_string::to_string(infos),
                 traa::main::obj_string::to_string(count));

  return traa_error::TRAA_ERROR_NONE;
}

int engine::free_screen_source_info(traa_screen_source_info infos[], int count) {
  LOG_API_ARGS_2(traa::main::obj_string::to_string(infos),
                 traa::main::obj_string::to_string(count));

  return traa_error::TRAA_ERROR_NONE;
}
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

} // namespace main
} // namespace traa
