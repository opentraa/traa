#include "main/engine.h"

#include "base/devices/screen/enumerator.h"
#include "base/logger.h"

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

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
int engine::enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                    const unsigned int external_flags,
                                    traa_screen_source_info **infos, int *count) {
  return base::screen_source_info_enumerator::enum_screen_source_info(icon_size, thumbnail_size,
                                                                      external_flags, infos, count);
}

int engine::free_screen_source_info(traa_screen_source_info infos[], int count) {
  return base::screen_source_info_enumerator::free_screen_source_info(infos, count);
}

int engine::create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                            int *data_size, traa_size *actual_size) {
  return base::screen_source_info_enumerator::create_snapshot(source_id, snapshot_size, data,
                                                              data_size, actual_size);
}

void engine::free_snapshot(uint8_t *data) {
  return base::screen_source_info_enumerator::free_snapshot(data);
}

#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

} // namespace main
} // namespace traa
