#include "base/devices/screen/linux/x11/x_window_list_utils.h"

#include "base/arch.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/linux/x11/x_error_trap.h"
#include "base/devices/screen/linux/x11/x_server_pixel_buffer.h"
#include "base/devices/screen/linux/x11/x_window_property.h"
#include "base/devices/screen/utils.h"

#include "base/log/logger.h"

#include <libyuv/scale_argb.h>

#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/composite.h>

#include <algorithm>
#include <string>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRAA_DUMP_IMAGES 0

namespace traa {
namespace base {

inline namespace x11 {

class defer_x_free {
public:
  explicit defer_x_free(void *data) : data_(data) {}
  ~defer_x_free() {
    if (data_)
      XFree(data_);
  }

private:
  void *const data_;
};

// Iterates through `window` hierarchy to find first visible window, i.e. one
// that has WM_STATE property set to NormalState.
// See http://tronche.com/gui/x/icccm/sec-4.html#s-4.1.3.1 .
::Window get_application_window(x_atom_cache *cache, ::Window window) {
  int32_t state = x_window_list_utils::get_window_state(cache, window);
  if (state == NormalState) {
    // Window has WM_STATE==NormalState. Return it.
    return window;
  } else if (state == IconicState) {
    // Window is in minimized. Skip it.
    return 0;
  }

  // If the window is in WithdrawnState then look at all of its children.
  ::Window root, parent;
  ::Window *children;
  unsigned int num_children;
  if (!XQueryTree(cache->display(), window, &root, &parent, &children, &num_children)) {
    LOG_ERROR("failed to query for child windows although window does not have a valid WM_STATE.");
    return 0;
  }
  ::Window app_window = 0;
  for (unsigned int i = 0; i < num_children; ++i) {
    app_window = get_application_window(cache, children[i]);
    if (app_window)
      break;
  }

  if (children)
    XFree(children);
  return app_window;
}

// Returns true if the `window` is a desktop element.
bool is_desktop_element(x_atom_cache *cache, ::Window window) {
  if (window == 0)
    return false;

  // First look for _NET_WM_WINDOW_TYPE. The standard
  // (http://standards.freedesktop.org/wm-spec/latest/ar01s05.html#id2760306)
  // says this hint *should* be present on all windows, and we use the existence
  // of _NET_WM_WINDOW_TYPE_NORMAL in the property to indicate a window is not
  // a desktop element (that is, only "normal" windows should be shareable).
  x_window_property<uint32_t> window_type(cache->display(), window, cache->window_type());
  if (window_type.is_valid() && window_type.size() > 0) {
    uint32_t *end = window_type.data() + window_type.size();
    bool is_normal = (end != std::find(window_type.data(), end, cache->window_type_normal()));
    return !is_normal;
  }

  // Fall back on using the hint.
  XClassHint class_hint;
  Status status = XGetClassHint(cache->display(), window, &class_hint);
  if (status == 0) {
    // No hints, assume this is a normal application window.
    return false;
  }

  defer_x_free free_res_name(class_hint.res_name);
  defer_x_free free_res_class(class_hint.res_class);
  return strcmp("gnome-panel", class_hint.res_name) == 0 ||
         strcmp("desktop_window", class_hint.res_name) == 0;
}

bool is_window_visible(Display *display, ::Window window) {
  x_error_trap error_trap(display);
  XWindowAttributes attrs;
  if (!XGetWindowAttributes(display, window, &attrs) ||
      error_trap.get_last_error_and_disable() != 0) {
    LOG_ERROR("failed to get window attributes for window {}", window);
    return false;
  }

  return attrs.map_state == IsViewable;
}

bool is_window_minimized(x_atom_cache *cache, ::Window window) {
  int32_t state = x_window_list_utils::get_window_state(cache, window);
  return state == IconicState;
}

bool is_window_fullscreen(x_atom_cache *cache, ::Window window) {
  //   x_window_property<Atom> window_state(cache->display(), window, cache->wm_state());
  //   if (!window_state.is_valid()) {
  //     return false;
  //   }

  //   Atom *end = window_state.data() + window_state.size();
  //   return end != std::find(window_state.data(), end, cache->wm_state_fullscreen());
  return false;
}

bool get_window_image_data(x_atom_cache *cache, ::Window window, const desktop_rect &window_rect,
                           const traa_size &target_size, uint8_t **data, traa_size &scaled_size) {
  // use XImage to get the thumbnail directly is faster than using x_server_pixel_buffer for now,
  // coz we can use image->data directly to scale the image, but we need to handle the data format
  // by ourselves. so we need to implement a new function to get the scaled image data from
  // x_server_pixel_buffer in the fast path.
#define USE_XIMAGE_DIRECTLY 0

#if USE_XIMAGE_DIRECTLY
  XImage *image = nullptr;
  {
    x_error_trap error_trap(cache->display());
    if (!(image = XGetImage(cache->display(), window, 0, 0, window_rect.width(),
                            window_rect.height(), AllPlanes, ZPixmap)) ||
        error_trap.get_last_error_and_disable() != 0) {
      LOG_ERROR("failed to get image for window {}", window);
      return false;
    }
  }

#if TRAA_DUMP_IMAGES
  // create a file to save the image
  save_ximage_to_ppm((std::string("origin_image_") + std::to_string(window) + ".ppm").c_str(),
                     image);
#endif // TRAA_DUMP_IMAGES

  scaled_size =
      calc_scaled_size(desktop_size(image->width, image->height), target_size).to_traa_size();

  // to get more information about XImage, see
  // https://tronche.com/gui/x/xlib/graphics/images.html#XImage
  const size_t k_bytes_per_pixel = image->bits_per_pixel / 8;
  *data = new uint8_t[scaled_size.width * scaled_size.height * k_bytes_per_pixel];
  if (!*data) {
    LOG_ERROR("failed to allocate memory for thumbnail data");
    XDestroyImage(image);
    return false;
  }

  // use libyuv to scale the image
  libyuv::ARGBScale(reinterpret_cast<uint8_t *>(image->data), image->bytes_per_line, image->width,
                    image->height, *data, scaled_size.width * k_bytes_per_pixel, scaled_size.width,
                    scaled_size.height, libyuv::kFilterBox);

  XDestroyImage(image);
#else
  XCompositeRedirectWindow(cache->display(), window, CompositeRedirectAutomatic);

  x_server_pixel_buffer pixel_buffer;
  if (!pixel_buffer.init(cache, window)) {
    LOG_ERROR("failed to init pixel buffer for window {}", window);
    return false;
  }

  basic_desktop_frame frame(pixel_buffer.window_size());
  pixel_buffer.synchronize();
  if (!pixel_buffer.capture_rect(desktop_rect::make_size(frame.size()), &frame)) {
    LOG_ERROR("failed to capture rect for window {}", window);
    return false;
  }

  scaled_size = calc_scaled_size(frame.size(), target_size).to_traa_size();
  *data = new uint8_t[scaled_size.width * scaled_size.height * desktop_frame::k_bytes_per_pixel];
  if (!*data) {
    LOG_ERROR("failed to allocate memory for thumbnail data");
    return false;
  }

  // use libyuv to scale the image
  libyuv::ARGBScale(frame.data(), frame.stride(), frame.size().width(), frame.size().height(),
                    *data, scaled_size.width * desktop_frame::k_bytes_per_pixel, scaled_size.width,
                    scaled_size.height, libyuv::kFilterBox);
#endif // USE_XIMAGE_DIRECTLY

  return true;
}

pid_t get_pid_by_window(Display *display, ::Window window) {
  Atom pid_atom = XInternAtom(display, "_NET_WM_PID", True);
  if (pid_atom == None) {
    return -1;
  }

  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = nullptr;

  x_error_trap error_trap(display);
  if ((XGetWindowProperty(display, window, pid_atom, 0, 1, False, XA_CARDINAL, &actual_type,
                          &actual_format, &nitems, &bytes_after, &prop) != Success) ||
      error_trap.get_last_error_and_disable() != 0) {
    return -1;
  }

  if (nitems == 0) {
    return -1;
  }

  pid_t pid = *(pid_t *)prop;
  XFree(prop);
  return pid;
}

std::string get_process_path(pid_t pid) {
  char path[1024];
  snprintf(path, sizeof(path), "/proc/%d/exe", pid);
  char exe_path[1024];
  ssize_t len = readlink(path, exe_path, sizeof(exe_path) - 1);
  if (len == -1) {
    return "";
  }
  exe_path[len] = '\0';
  return std::string(exe_path);
}

bool get_window_icon(Display *display, ::Window window, std::vector<uint8_t> &icon_data, int &width,
                     int &height) {
  Atom icon_atom = XInternAtom(display, "_NET_WM_ICON", True);
  if (icon_atom == None) {
    return false;
  }

  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = nullptr;

  x_error_trap error_trap(display);
  if ((XGetWindowProperty(display, window, icon_atom, 0, (~0L), False, AnyPropertyType,
                          &actual_type, &actual_format, &nitems, &bytes_after, &prop) != Success) ||
      error_trap.get_last_error_and_disable() != 0) {
    return false;
  }

  if (nitems == 0) {
    LOG_ERROR("no icon data for window {}", window);
    return false;
  }

  // https://www.x.org/archive/X11R7.5/doc/man/man3/XGetWindowProperty.3.html
  // actual_format : If the returned format is 8, the returned data is represented as a char array.
  // If the returned format is 16, the returned data is represented as a short array and should be
  // cast to that type to obtain the elements. If the returned format is 32, the returned data is
  // represented as a long array and should be cast to that type to obtain the elements.
  //
  // nitems: Returns the actual number of 8-bit, 16-bit, or 32-bit items stored in the prop_return
  // data.
  //
  // prop: Returns the data in the specified format. If the returned format is 8, the returned data
  // is represented as a char array. If the returned format is 16, the returned data is represented
  // as a array of short int type and should be cast to that type to obtain the elements. If the
  // returned format is 32, the property data will be stored as an array of longs (which in a 64-bit
  // application will be 64-bit values that are padded in the upper 4 bytes).

  // https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html#id-1.6.13
  //
  // _NET_WM_ICON CARDINAL[][2+n]/32
  // This is an array of possible icons for the client. This specification does not stipulate what
  // size these icons should be, but individual desktop environments or toolkits may do so. The
  // Window Manager MAY scale any of these icons to an appropriate size.
  //
  // This is an array of 32bit packed CARDINAL ARGB with high byte being A, low
  // byte being B. The first two cardinals are width, height. Data is in rows,
  // left to right and top to bottom.

  // Note that the icon data is stored as an array of 32-bit values, so the actual format should be
  // 32 bits.
  if (actual_format != 32) {
    LOG_ERROR("unexpected icon format: {}", actual_format);
    XFree(prop);
    return false;
  }

  // Note that the prop data is stored as an array of longs(which in a 64-bit application will be
  // 64-bit values that are padded in the upper 4 bytes), and the first two elements are width and
  // height. So we just cast the prop to unsigned long and get the width and height(no matter
  // sizeof(unsigned long) is 4 or 8).
  unsigned long *data = reinterpret_cast<unsigned long *>(prop);
  width = static_cast<int>(data[0]);
  height = static_cast<int>(data[1]);

  // Note that the prop data is stored as an array of longs(which in a 64-bit application will be
  // 64-bit values that are padded in the upper 4 bytes), which means we need to implement two
  // versions of the copy function, fast and slow. The fast version is used when the architecture is
  // in 32-bit(directly copy the whole memory block), and the slow version is used when the
  // architecture is in 64-bit(copy the memory block element by element).

#if defined(TRAA_ARCH_32_BITS)
  icon_data.assign(prop + 2 * sizeof(unsigned long), prop + nitems * sizeof(unsigned long));
#elif defined(TRAA_ARCH_64_BITS)
  // TODO(@sylar): this can be optimized by using some SIMD instructions.
  icon_data.resize(width * height * desktop_frame::k_bytes_per_pixel);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned long pixel = data[2 + y * width + x];
      icon_data[(y * width + x) * desktop_frame::k_bytes_per_pixel + 3] =
          (pixel >> (24)) & 0xff; // B
      icon_data[(y * width + x) * desktop_frame::k_bytes_per_pixel + 2] =
          (pixel >> (16)) & 0xff; // G
      icon_data[(y * width + x) * desktop_frame::k_bytes_per_pixel + 1] =
          (pixel >> (8)) & 0xff;                                                        // R
      icon_data[(y * width + x) * desktop_frame::k_bytes_per_pixel + 0] = pixel & 0xff; // A
    }
  }
#endif

  XFree(prop);
  return true;
}

} // namespace x11

// generatd by copilot
void x_window_list_utils::save_ximage_to_ppm(const char *filename, XImage *image) {
  FILE *fp = fopen(filename, "wb+");
  if (!fp) {
    LOG_ERROR("unable to open file");
    return;
  }

  // Write PPM header.
  fprintf(fp, "P6\n%d %d\n255\n", image->width, image->height);

  // Write pixel data.
  for (int y = 0; y < image->height; y++) {
    for (int x = 0; x < image->width; x++) {
      // Get pixel value.
      unsigned long pixel = XGetPixel(image, x, y);
      // Extract RGB color components.
      unsigned char r = (pixel & image->red_mask) >> 16;
      unsigned char g = (pixel & image->green_mask) >> 8;
      unsigned char b = (pixel & image->blue_mask);
      // Write RGB data.
      fwrite(&r, sizeof(unsigned char), 1, fp);
      fwrite(&g, sizeof(unsigned char), 1, fp);
      fwrite(&b, sizeof(unsigned char), 1, fp);
    }
  }

  fclose(fp);
}

// generatd by copilot
void x_window_list_utils::save_pixel_to_ppm(const char *filename, const uint8_t *data, int width,
                                            int height) {
  FILE *fp = fopen(filename, "wb+");
  if (!fp) {
    LOG_ERROR("unable to open file");
    return;
  }

  // Write PPM header.
  fprintf(fp, "P6\n%d %d\n255\n", width, height);

  // Write pixel data.
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // Get pixel value, the data is in BGRA format.
      unsigned char b = data[(y * width + x) * 4 + 0];
      unsigned char g = data[(y * width + x) * 4 + 1];
      unsigned char r = data[(y * width + x) * 4 + 2];
      // Write RGB data.
      fwrite(&r, sizeof(unsigned char), 1, fp);
      fwrite(&g, sizeof(unsigned char), 1, fp);
      fwrite(&b, sizeof(unsigned char), 1, fp);
    }
  }

  fclose(fp);
}

int32_t x_window_list_utils::get_window_state(x_atom_cache *cache, ::Window window) {
  // Get WM_STATE property of the window.
  x_window_property<uint32_t> window_state(cache->display(), window, cache->wm_state());

  // WM_STATE is considered to be set to WithdrawnState when it missing.
  return window_state.is_valid() ? *window_state.data() : WithdrawnState;
}

bool x_window_list_utils::get_window_title(Display *display, ::Window window, std::string *title) {
  int status;
  bool result = false;
  XTextProperty window_name;
  window_name.value = nullptr;
  if (window) {
    status = XGetWMName(display, window, &window_name);
    if (status && window_name.value && window_name.nitems) {
      int cnt;
      char **list = nullptr;
      status = Xutf8TextPropertyToTextList(display, &window_name, &list, &cnt);
      if (status >= Success && cnt && *list) {
        if (cnt > 1) {
          LOG_INFO("window has {} text properties, only using the first one.", cnt);
        }
        *title = *list;
        result = true;
      }
      if (list)
        XFreeStringList(list);
    }
    if (window_name.value)
      XFree(window_name.value);
  }
  return result;
}

bool x_window_list_utils::get_window_rect(::Display *display, ::Window window, desktop_rect *rect,
                                          XWindowAttributes *attributes) {
  XWindowAttributes local_attributes;
  int offset_x;
  int offset_y;
  if (attributes == nullptr) {
    attributes = &local_attributes;
  }

  {
    x_error_trap error_trap(display);
    if (!XGetWindowAttributes(display, window, attributes) ||
        error_trap.get_last_error_and_disable() != 0) {
      return false;
    }
  }
  *rect =
      desktop_rect::make_xywh(attributes->x, attributes->y, attributes->width, attributes->height);

  {
    x_error_trap error_trap(display);
    ::Window child;
    if (!XTranslateCoordinates(display, window, attributes->root, -rect->left(), -rect->top(),
                               &offset_x, &offset_y, &child) ||
        error_trap.get_last_error_and_disable() != 0) {
      return false;
    }
  }
  rect->translate(offset_x, offset_y);
  return true;
}

int x_window_list_utils::enum_windows(const traa_size icon_size, const traa_size thumbnail_size,
                                      const unsigned int external_flags,
                                      std::vector<traa_screen_source_info> &infos) {

  Display *display = XOpenDisplay(NULL);
  if (!display) {
    LOG_ERROR("failed to open display");
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  ::Window root = DefaultRootWindow(display);
  if (!root) {
    LOG_ERROR("failed to get root window");
    XCloseDisplay(display);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  x_atom_cache atom_cache(display);

  ::Window parent;
  ::Window *children;
  unsigned int num_children;
  if (!XQueryTree(display, root, &root, &parent, &children, &num_children)) {
    LOG_ERROR("failed to query for child windows");
    XCloseDisplay(display);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  for (unsigned int i = 0; i < num_children; ++i) {
    traa_screen_source_info window_info;
    window_info.id = static_cast<int64_t>(reinterpret_cast<std::uintptr_t>(children[i]));
    window_info.is_window = true;
    window_info.is_minimized = is_window_minimized(&atom_cache, children[i]);
    window_info.is_maximized = is_window_fullscreen(&atom_cache, children[i]);

    ::Window app_window = get_application_window(&atom_cache, children[i]);
    if (app_window && !is_desktop_element(&atom_cache, app_window)) {
      if (!is_window_visible(display, app_window)) {
        continue;
      }

      desktop_rect rect;
      XWindowAttributes attrs;
      if (!get_window_rect(display, app_window, &rect, &attrs)) {
        LOG_ERROR("failed to get window rect for window {}", app_window);
        continue;
      }

      window_info.rect = rect.to_traa_rect();

      std::string str_title;
      if ((!get_window_title(display, app_window, &str_title) || str_title.empty()) &&
          !(external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNTITLED)) {
        continue;
      }

      if (!str_title.empty()) {
        strncpy(const_cast<char *>(window_info.title), str_title.c_str(),
                std::min(sizeof(window_info.title) - 1, str_title.size()));
      }

      // get process path
      bool has_process_path = false;
      pid_t pid = get_pid_by_window(display, app_window);
      if (pid != -1) {
        std::string process_path = get_process_path(pid);
        if (!process_path.empty()) {
          has_process_path = true;
          strncpy(const_cast<char *>(window_info.process_path), process_path.c_str(),
                  std::min(sizeof(window_info.process_path) - 1, process_path.size()));
        }
      }

      if ((external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_NOPROCESS_PATH) && !has_process_path) {
        continue;
      }

      // only get thumbnail for the window when thumbnail_size is set.
      if (thumbnail_size.width > 0 && thumbnail_size.height > 0 &&
          !get_window_image_data(&atom_cache, app_window, rect, thumbnail_size,
                                 const_cast<uint8_t **>(&window_info.thumbnail_data),
                                 window_info.thumbnail_size)) {
        LOG_ERROR("get thumbnail data failed");
        continue;
      }

#if TRAA_DUMP_IMAGES
      // create a file to save the thumbnail
      if (window_info.thumbnail_data) {
        save_pixel_to_ppm(
            (std::string("thumbnail_") + std::to_string(window_info.id) + ".ppm").c_str(),
            window_info.thumbnail_data, window_info.thumbnail_size.width,
            window_info.thumbnail_size.height);
      }
#endif // TRAA_DUMP_IMAGES

      // only get icon for the window when icon_size is set.
      if (icon_size.width > 0 && icon_size.height > 0) {
        std::vector<uint8_t> icon_data;
        int icon_width, icon_height;
        if (get_window_icon(display, app_window, icon_data, icon_width, icon_height)) {
#if TRAA_DUMP_IMAGES
          // create a file to save the origin icon
          save_pixel_to_ppm(
              (std::string("origin_icon_") + std::to_string(window_info.id) + ".ppm").c_str(),
              icon_data.data(), icon_width, icon_height);
#endif // TRAA_DUMP_IMAGES

          traa_size scaled_icon_size =
              calc_scaled_size(desktop_size(icon_width, icon_height), icon_size).to_traa_size();
          window_info.icon_data = new uint8_t[scaled_icon_size.width * scaled_icon_size.height *
                                              desktop_frame::k_bytes_per_pixel];
          if (!window_info.icon_data) {
            LOG_ERROR("failed to allocate memory for icon data");
            continue;
          }

          libyuv::ARGBScale(icon_data.data(), icon_width * desktop_frame::k_bytes_per_pixel,
                            icon_width, icon_height, const_cast<uint8_t *>(window_info.icon_data),
                            scaled_icon_size.width * desktop_frame::k_bytes_per_pixel,
                            scaled_icon_size.width, scaled_icon_size.height, libyuv::kFilterBox);

          window_info.icon_size = scaled_icon_size;
        }

#if TRAA_DUMP_IMAGES
        // create a file to save the icon
        if (window_info.icon_data) {
          save_pixel_to_ppm(
              (std::string("icon_") + std::to_string(window_info.id) + ".ppm").c_str(),
              window_info.icon_data, window_info.icon_size.width, window_info.icon_size.height);
        }
#endif // TRAA_DUMP_IMAGES
      }

      infos.push_back(window_info);
    }
  }

  XFree(children);

  XCloseDisplay(display);

  return traa_error::TRAA_ERROR_NONE;
}

int x_window_list_utils::enum_screens(const traa_size thumbnail_size,
                                      const unsigned int external_flags,
                                      std::vector<traa_screen_source_info> &infos) {
  // connect to the X server
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    LOG_ERROR("failed to open display");
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  // check if the Xrandr extension is available
  int event_base, error_base;
  if (!XRRQueryExtension(display, &event_base, &error_base)) {
    LOG_ERROR("Xrandr extension is not available");
    XCloseDisplay(display);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  x_atom_cache atom_cache(display);

  // get the root window
  ::Window root = XDefaultRootWindow(display);

  x_server_pixel_buffer pixel_buffer;
  if (thumbnail_size.width > 0 && thumbnail_size.height > 0) {
    // must init the pixel buffer before calling XRRGetMonitors, ohterwise init will raise an
    // BadMatch error.I don't know why, just do it.

    if (!pixel_buffer.init(&atom_cache, root)) {
      LOG_ERROR("failed to init pixel buffer for window {}", root);
      return false;
    }

    pixel_buffer.synchronize();

#if TRAA_DUMP_IMAGES
    // capture the whole screen
    basic_desktop_frame full_screen_frame(pixel_buffer.window_size());

    if (!pixel_buffer.capture_rect(desktop_rect::make_size(full_screen_frame.size()),
                                   &full_screen_frame)) {
      LOG_ERROR("failed to capture rect for screen {}", root);
      return false;
    }

    // create a file to save the full screen
    save_pixel_to_ppm("full_screen.ppm", full_screen_frame.data(), full_screen_frame.size().width(),
                      full_screen_frame.size().height());
#endif // TRAA_DUMP_IMAGES
  }

  int monitor_count = 0;
  XRRMonitorInfo *monitors = XRRGetMonitors(display, root, True, &monitor_count);
  if (!monitors) {
    LOG_ERROR("failed to get monitors");
    XCloseDisplay(display);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  for (int i = 0; i < monitor_count; ++i) {
    traa_screen_source_info screen_info;
    screen_info.id = static_cast<int64_t>(i);
    screen_info.is_window = false;
    screen_info.is_primary = monitors[i].primary;

    auto screen_rect = desktop_rect::make_xywh(monitors[i].x, monitors[i].y, monitors[i].width,
                                               monitors[i].height);
    screen_info.rect = screen_rect.to_traa_rect();

    // get name of the screen
    // TODO(@sylar): the name from XGetAtomName is not correct, need to find a way to get the
    // friendly name.
    char *monitor_name = XGetAtomName(display, monitors[i].name);
    if (monitor_name) {
      strncpy(const_cast<char *>(screen_info.title), monitor_name,
              std::min(sizeof(screen_info.title) - 1, strlen(monitor_name)));
      XFree(monitor_name);
    }

    if (thumbnail_size.width > 0 && thumbnail_size.height > 0) {
      // copy the pixel data from the full screen frame by the screen rect, and scale it to the
      // thumbnail size.

      traa_size scaled_size = calc_scaled_size(screen_rect.size(), thumbnail_size).to_traa_size();
      screen_info.thumbnail_data =
          new uint8_t[scaled_size.width * scaled_size.height * desktop_frame::k_bytes_per_pixel];
      if (!screen_info.thumbnail_data) {
        LOG_ERROR("failed to allocate memory for thumbnail data");
        continue;
      }

      basic_desktop_frame frame(screen_rect.size());
      frame.set_top_left(screen_rect.top_left());

      if (!pixel_buffer.capture_rect(screen_rect, &frame)) {
        LOG_ERROR("failed to capture rect for screen {}", root);
        continue;
      }

      // use libyuv to scale the image
      libyuv::ARGBScale(frame.data(), frame.stride(), frame.size().width(), frame.size().height(),
                        const_cast<uint8_t *>(screen_info.thumbnail_data),
                        scaled_size.width * desktop_frame::k_bytes_per_pixel, scaled_size.width,
                        scaled_size.height, libyuv::kFilterBox);

      screen_info.thumbnail_size = scaled_size;
    }

#if TRAA_DUMP_IMAGES
    // create a file to save the thumbnail
    if (screen_info.thumbnail_data) {
      save_pixel_to_ppm(
          (std::string("screenshot_") + std::to_string(screen_info.id) + ".ppm").c_str(),
          screen_info.thumbnail_data, screen_info.thumbnail_size.width,
          screen_info.thumbnail_size.height);
    }
#endif // TRAA_DUMP_IMAGES

    infos.push_back(screen_info);
  }

  XRRFreeMonitors(monitors);
  XCloseDisplay(display);

  return traa_error::TRAA_ERROR_NONE;
}

int x_window_list_utils::enum_screen_source_info(const traa_size icon_size,
                                                 const traa_size thumbnail_size,
                                                 const unsigned int external_flags,
                                                 traa_screen_source_info **infos, int *count) {
  std::vector<traa_screen_source_info> sources;

  if (!(external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN)) {
    enum_screens(thumbnail_size, external_flags, sources);
  }

  if (!(external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_WINDOW)) {
    enum_windows(icon_size, thumbnail_size, external_flags, sources);
  }

  if (sources.size() == 0) {
    return traa_error::TRAA_ERROR_NONE;
  }

  *count = static_cast<int>(sources.size());
  *infos = reinterpret_cast<traa_screen_source_info *>(new traa_screen_source_info[sources.size()]);
  if (*infos == nullptr) {
    LOG_ERROR("alloca memroy for infos failed");
    return traa_error::TRAA_ERROR_OUT_OF_MEMORY;
  }

  for (size_t i = 0; i < sources.size(); ++i) {
    auto &source_info = sources[i];
    auto &dest_info = (*infos)[i];
    memcpy(reinterpret_cast<void *>(&dest_info), reinterpret_cast<void *>(&source_info),
           sizeof(traa_screen_source_info));
    if (std::strlen(source_info.title) > 0) {
      strncpy(const_cast<char *>(dest_info.title), source_info.title,
              std::min(sizeof(dest_info.title) - 1, std::strlen(source_info.title)));
    }

    if (std::strlen(source_info.process_path) > 0) {
      strncpy(const_cast<char *>(dest_info.process_path), source_info.process_path,
              std::min(sizeof(dest_info.process_path) - 1, std::strlen(source_info.process_path)));
    }
  }

  return TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa