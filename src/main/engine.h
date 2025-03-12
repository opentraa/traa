#ifndef TRAA_MAIN_ENGINE_H_
#define TRAA_MAIN_ENGINE_H_

#include <traa/traa.h>

#include <string>
#include <vector>

#include "base/disallow.h"
#include "base/thread/callback.h"

namespace traa {
namespace main {

class engine : public base::support_weak_callback {
  DISALLOW_COPY_AND_ASSIGN(engine);

public:
  engine();
  ~engine();

  int init(const traa_config *config);

  int set_event_handler(const traa_event_handler *handler);

  int enum_device_info(traa_device_type type, traa_device_info **infos, int *count);

  int free_device_info(traa_device_info infos[]);

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
  static int enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                     const unsigned int external_flags,
                                     traa_screen_source_info **infos, int *count);

  static int free_screen_source_info(traa_screen_source_info infos[], int count);

  static int create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                             int *data_size, traa_size *actual_size);

  static void free_snapshot(uint8_t *data);
#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

private:
};

} // namespace main
} // namespace traa

#endif // TRAA_MAIN_ENGINE_H_