#include "base/devices/screen/enumerator.h"

#include "traa/error.h"

namespace traa {
namespace base {

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
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
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

} // namespace base
} // namespace traa