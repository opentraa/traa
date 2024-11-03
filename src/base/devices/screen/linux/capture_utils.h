#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_CAPTURE_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_CAPTURE_UTILS_H_

#include "base/disallow.h"

namespace traa {
namespace base {

class capture_utils {
  DISALLOW_COPY_AND_ASSIGN(capture_utils);

public:
#if defined(TRAA_ENABLE_X11) || defined(TRAA_ENABLE_WAYLAND)
  static bool is_running_under_wayland();
#endif // TRAA_ENABLE_X11 || TRAA_ENABLE_WAYLAND
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_CAPTURE_UTILS_H_