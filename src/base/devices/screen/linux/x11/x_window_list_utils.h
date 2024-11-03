#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_X11_WINDOW_LIST_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_X11_WINDOW_LIST_UTILS_H_

#include <traa/base.h>

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/linux/x11/x_atom_cache.h"
#include "base/disallow.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string>
#include <vector>

namespace traa {
namespace base {

class x_window_list_utils {
  DISALLOW_COPY_AND_ASSIGN(x_window_list_utils);

public:
  static void save_ximage_to_ppm(const char *filename, XImage *image);

  static void save_pixel_to_ppm(const char *filename, const uint8_t *data, int width, int height);

  // Returns WM_STATE property of the `window`. This function returns
  // WithdrawnState if the `window` is missing.
  static int32_t get_window_state(x_atom_cache *cache, ::Window window);

  static bool get_window_title(Display *display, ::Window window, std::string *title);

  // Returns the rectangle of the `window` in the coordinates of `display`. This
  // function returns false if native APIs failed. If `attributes` is provided, it
  // will be filled with the attributes of `window`. The `rect` is in system
  // coordinate, i.e. the primary monitor always starts from (0, 0).
  static bool get_window_rect(::Display *display, ::Window window, desktop_rect *rect,
                              XWindowAttributes *attributes /* = nullptr */);

  static int enum_windows(const traa_size icon_size, const traa_size thumbnail_size,
                          const unsigned int external_flags,
                          std::vector<traa_screen_source_info> &infos);

  static int enum_screens(const traa_size thumbnail_size, const unsigned int external_flags,
                          std::vector<traa_screen_source_info> &infos);

  static int enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                     const unsigned int external_flags,
                                     traa_screen_source_info **infos, int *count);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_X11_WINDOW_LIST_UTILS_H_