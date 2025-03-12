#include "base/devices/screen/enumerator.h"

#include "base/devices/screen/linux/capture_utils.h"

#if defined(TRAA_ENABLE_X11)
#include "base/devices/screen/linux/x11/x_window_list_utils.h"
#endif // TRAA_ENABLE_X11

namespace traa {
namespace base {

int screen_source_info_enumerator::enum_screen_source_info(const traa_size icon_size,
                                                           const traa_size thumbnail_size,
                                                           const unsigned int external_flags,
                                                           traa_screen_source_info **infos,
                                                           int *count) {

#define FORCE_X11 0

#if defined(TRAA_ENABLE_X11)
#if !(FORCE_X11) // do test reason
  if (!capture_utils::is_running_under_wayland()) {
#endif // FORCE_X11
    return x_window_list_utils::enum_screen_source_info(icon_size, thumbnail_size, external_flags,
                                                        infos, count);
#if !(FORCE_X11)
  }
#endif // FORCE_X11
#endif // TRAA_ENABLE_X11

  return traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED;
}

int screen_source_info_enumerator::create_snapshot(const int64_t source_id,
                                                   const traa_size snapshot_size, uint8_t **data,
                                                   int *data_size, traa_size *actual_size) {
  return traa_error::TRAA_ERROR_NOT_IMPLEMENTED;
}

} // namespace base
} // namespace traa
