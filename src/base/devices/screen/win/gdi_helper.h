#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_GDI_HELPER_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_GDI_HELPER_H_

#include <traa/base.h>

#include <Windows.h>

namespace traa {
namespace base {

class gdi_helper {
public:
  static bool get_image(HWND window, const traa_size &target_size, uint8_t **data,
                        traa_size &scaled_size);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_GDI_HELPER_H_