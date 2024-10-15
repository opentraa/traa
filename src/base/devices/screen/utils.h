#ifndef TRAA_BASE_DEVICES_SCREEN_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_UTILS_H_

#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

desktop_size calc_scaled_size(const desktop_size &source, const desktop_size &dest);

#if defined(_WIN32)
void dump_bmp(const uint8_t *data, const traa_size &size, const char *file_name);
#endif // _WIN32

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_UTILS_H_