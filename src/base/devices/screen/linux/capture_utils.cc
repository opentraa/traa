#include "base/devices/screen/linux/capture_utils.h"

#include <cstdlib>
#include <cstring>

namespace traa {
namespace base {

#if !defined(TRAA_ENABLE_X11) && !defined(TRAA_ENABLE_WAYLAND)
static_assert(true, "At least one of TRAA_ENABLE_X11 or TRAA_ENABLE_WAYLAND must be defined");
#endif

#if defined(TRAA_ENABLE_X11) || defined(TRAA_ENABLE_WAYLAND)
bool capture_utils::is_running_under_wayland() {
  const char *xdg_session_type = getenv("XDG_SESSION_TYPE");
  if (!xdg_session_type || strncmp(xdg_session_type, "wayland", 7) != 0)
    return false;

  if (!(getenv("WAYLAND_DISPLAY")))
    return false;

  return true;
}
#endif // TRAA_ENABLE_X11 || TRAA_ENABLE_WAYLAND

} // namespace base
} // namespace traa