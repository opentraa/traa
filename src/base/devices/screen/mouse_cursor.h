#ifndef TRAA_BASE_DEVICES_SCREEN_MOUSE_CURSOR_H_
#define TRAA_BASE_DEVICES_SCREEN_MOUSE_CURSOR_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"

#include <memory>

namespace traa {
namespace base {

class mouse_cursor {
public:
  mouse_cursor();

  // Takes ownership of `image`. `hotspot` must be within `image` boundaries.
  mouse_cursor(desktop_frame *image, const desktop_vector &hotspot);

  ~mouse_cursor();

  mouse_cursor(const mouse_cursor &) = delete;
  mouse_cursor &operator=(const mouse_cursor &) = delete;

  static mouse_cursor *copy_of(const mouse_cursor &cursor);

  void set_image(desktop_frame *image) { image_.reset(image); }
  const desktop_frame *image() const { return image_.get(); }

  void set_hotspot(const desktop_vector &hotspot) { hotspot_ = hotspot; }
  const desktop_vector &hotspot() const { return hotspot_; }

private:
  std::unique_ptr<desktop_frame> image_;
  desktop_vector hotspot_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_MOUSE_CURSOR_H_