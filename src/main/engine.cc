#include "main/engine.h"

#include "base/devices/screen/enumerator.h"
#include "base/log/logger.h"

#include "main/utils/obj_string.h"

namespace traa {
namespace main {

engine::engine() { LOG_API_ARGS_0(); }

engine::~engine() { LOG_API_ARGS_0(); }

int engine::init(const traa_config *config) { return traa_error::TRAA_ERROR_NONE; }

int engine::set_event_handler(const traa_event_handler *handler) {
  return traa_error::TRAA_ERROR_NONE;
}

int engine::enum_device_info(traa_device_type type, traa_device_info **infos, int *count) {
  return traa_error::TRAA_ERROR_NONE;
}

int engine::free_device_info(traa_device_info infos[]) { return traa_error::TRAA_ERROR_NONE; }

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
int engine::enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                    const unsigned int external_flags,
                                    traa_screen_source_info **infos, int *count) {
  return base::screen_source_info_enumerator::enum_screen_source_info(icon_size, thumbnail_size,
                                                                      external_flags, infos, count);
}

int engine::free_screen_source_info(traa_screen_source_info infos[], int count) {
  return base::screen_source_info_enumerator::free_screen_source_info(infos, count);
}
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

} // namespace main
} // namespace traa
