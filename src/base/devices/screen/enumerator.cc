#include "base/devices/screen/enumerator.h"

#include "traa/error.h"

namespace traa {
namespace base {

int screen_source_info_enumerator::free_screen_source_info(traa_screen_source_info infos[],
                                                           int count) {
  for (int i = 0; i < count; i++) {
    if (infos[i].icon_data != nullptr) {
      delete[] infos[i].icon_data;
    }

    if (infos[i].thumbnail_data != nullptr) {
      delete[] infos[i].thumbnail_data;
    }
  }

  delete[] infos;

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa