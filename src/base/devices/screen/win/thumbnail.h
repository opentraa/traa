#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_

#include <traa/base.h>

#include <windows.h>

namespace traa {
namespace base {

bool get_thumbnail_data_from_gdi(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                                 traa_size &size);

bool get_thumbnail_data(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                        traa_size &size);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_