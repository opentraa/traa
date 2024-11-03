#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_WINDOW_PROPERTY_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_WINDOW_PROPERTY_H_

#include <X11/X.h>
#include <X11/Xlib.h>

namespace traa {
namespace base {

class x_window_property_base {
public:
  x_window_property_base(Display *display, Window window, Atom property, int expected_size);
  virtual ~x_window_property_base() {
    if (data_)
      XFree(data_);
  }

  x_window_property_base(const x_window_property_base &) = delete;
  x_window_property_base &operator=(const x_window_property_base &) = delete;

  // True if we got properly value successfully.
  bool is_valid() const { return is_valid_; }

  // Size and value of the property.
  size_t size() const { return size_; }

protected:
  unsigned char *data_ = nullptr;

private:
  bool is_valid_ = false;
  unsigned long size_ = 0; // NOLINT: type required by XGetWindowProperty
};

// Convenience wrapper for XGetWindowProperty() results.
template <class property_type> class x_window_property : public x_window_property_base {
public:
  x_window_property(Display *display, const Window window, const Atom property)
      : x_window_property_base(display, window, property, sizeof(property_type)) {}
  ~x_window_property() override = default;

  x_window_property(const x_window_property &) = delete;
  x_window_property &operator=(const x_window_property &) = delete;

  const property_type *data() const { return reinterpret_cast<property_type *>(data_); }
  property_type *data() { return reinterpret_cast<property_type *>(data_); }
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_WINDOW_PROPERTY_H_