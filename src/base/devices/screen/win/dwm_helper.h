#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DWM_HELPER_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DWM_HELPER_H_

#include <traa/base.h>

#include <Windows.h>

namespace traa {
namespace base {

class dwm_helper {
public:
  static bool is_dwm_supported();
  static bool get_thubmnail_data_from_dwm(HWND dwm_window, HWND window,
                                          const traa_size &thumbnail_size, uint8_t **data,
                                          traa_size &size);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DWM_HELPER_H_