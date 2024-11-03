#include "base/devices/screen/linux/x11/x_window_property.h"

namespace traa {
namespace base {

x_window_property_base::x_window_property_base(Display *display, Window window, Atom property,
                                               int expected_size) {
  constexpr int bits_per_byte = 8;
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after; // NOLINT: type required by XGetWindowProperty
  int status = XGetWindowProperty(display, window, property, 0L, ~0L, False, AnyPropertyType,
                                  &actual_type, &actual_format, &size_, &bytes_after, &data_);
  if (status != Success) {
    data_ = nullptr;
    return;
  }
  if ((expected_size * bits_per_byte) != actual_format) {
    size_ = 0;
    return;
  }

  is_valid_ = true;
}

} // namespace base
} // namespace traa