#include "base/devices/screen/enumerator.h"

namespace traa {
namespace base {

int screen_source_info_enumerator::enum_screen_source_info(const traa_size icon_size,
                                                           const traa_size thumbnail_size,
                                                           const unsigned int external_flags,
                                                           traa_screen_source_info **infos,
                                                           int *count) {
  return TRAA_ERROR_NOT_IMPLEMENTED;
}

} // namespace base
} // namespace traa
