#include "base/devices/screen/mouse_cursor.h"

namespace traa {
namespace base {

mouse_cursor::mouse_cursor() {}

mouse_cursor::mouse_cursor(desktop_frame *image, const desktop_vector &hotspot)
    : image_(image), hotspot_(hotspot) {}

mouse_cursor::~mouse_cursor() {}

// static
mouse_cursor *mouse_cursor::copy_of(const mouse_cursor &cursor) {
  return cursor.image()
             ? new mouse_cursor(basic_desktop_frame::copy_of(*cursor.image()), cursor.hotspot())
             : new mouse_cursor();
}

} // namespace base
} // namespace traa