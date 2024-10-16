#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_

#include <traa/base.h>

#include "base/disallow.h"

#include <Windows.h>

namespace traa {
namespace base {

// capture utils
class capture_utils {
public:
  // gdi
  static void dump_bmp(const uint8_t *data, const traa_size &size, const char *file_name);

  static bool is_window_response(HWND window);

  static bool get_window_image_by_gdi(HWND window, const traa_size &target_size, uint8_t **data,
                                      traa_size &scaled_size);

  // dwm
  static bool is_dwm_supported();

  static bool get_window_image_by_dwm(HWND dwm_window, HWND window, const traa_size &target_size,
                                      uint8_t **data, traa_size &scaled_size);

private:
  DISALLOW_COPY_AND_ASSIGN(capture_utils);
};

// thumbnail
class thumbnail {
public:
  thumbnail();
  ~thumbnail();

  bool get_thumbnail_data(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                          traa_size &size);

private:
  HWND dwm_window_;

private:
  DISALLOW_COPY_AND_ASSIGN(thumbnail);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_