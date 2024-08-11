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

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
  int enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                              traa_screen_source_info **infos, int *count);

  int free_screen_source_info(traa_screen_source_info infos[], int count);
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

private:
};

} // namespace main
} // namespace traa

#endif // TRAA_MAIN_ENGINE_H_