#include "base/devices/screen/win/dwm_helper.h"

#include <mutex>

#include <dwmapi.h>

namespace traa {
namespace base {

bool dwm_helper::is_dwm_supported() {
#if defined(TRAA_SUPPORT_XP)
  static std::once_flag _flag;
  static std::atomic<bool> _supported(false);

  std::call_once(_flag, [&]() {
    HINSTANCE dwmapi = ::LoadLibraryW(L"dwmapi.dll");
    if (dwmapi != nullptr) {
      _supported.store(true, std::memory_order_release);
      ::FreeLibrary(dwmapi);
    }
  });

  return _supported.load(std::memory_order_acquire);
#else
  return true;
#endif
}

bool dwm_helper::get_thubmnail_data_from_dwm(HWND dwm_window, HWND window,
                                             const traa_size &thumbnail_size, uint8_t **data,
                                             traa_size &size) {
  if (!is_dwm_supported()) {
    return false;
  }
  return false;
}

} // namespace base
} // namespace traa