#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_OBJECT_GDI_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_OBJECT_GDI_H_

#include <windows.h>

namespace traa {
namespace base {

template <class T, class Traits> class scoped_object_gdi {
public:
  scoped_object_gdi() : handle_(NULL) {}
  explicit scoped_object_gdi(T object) : handle_(object) {}

  ~scoped_object_gdi() { Traits::close(handle_); }

  scoped_object_gdi(const scoped_object_gdi &) = delete;
  scoped_object_gdi &operator=(const scoped_object_gdi &) = delete;

  T get() { return handle_; }

  void set(T object) {
    if (handle_ && object != handle_)
      Traits::close(handle_);
    handle_ = object;
  }

  scoped_object_gdi &operator=(T object) {
    set(object);
    return *this;
  }

  T release() {
    T object = handle_;
    handle_ = NULL;
    return object;
  }

  operator T() { return handle_; }

private:
  T handle_;
};

// The traits class that uses DeleteObject() to close a handle.
template <typename T> class delete_object_traits {
public:
  delete_object_traits() = delete;
  delete_object_traits(const delete_object_traits &) = delete;
  delete_object_traits &operator=(const delete_object_traits &) = delete;

  // Closes the handle.
  static void close(T handle) {
    if (handle)
      ::DeleteObject(handle);
  }
};

// The traits class that uses DestroyCursor() to close a handle.
class destroy_cursor_traits {
public:
  destroy_cursor_traits() = delete;
  destroy_cursor_traits(const destroy_cursor_traits &) = delete;
  destroy_cursor_traits &operator=(const destroy_cursor_traits &) = delete;

  // Closes the handle.
  static void close(HCURSOR handle) {
    if (handle)
      ::DestroyCursor(handle);
  }
};

class destroy_icon_traits {
public:
  destroy_icon_traits() = delete;
  destroy_icon_traits(const destroy_icon_traits &) = delete;
  destroy_icon_traits &operator=(const destroy_icon_traits &) = delete;

  // Closes the handle.
  static void close(HICON handle) {
    if (handle)
      ::DestroyIcon(handle);
  }
};

typedef scoped_object_gdi<HBITMAP, delete_object_traits<HBITMAP>> scoped_bitmap;
typedef scoped_object_gdi<HCURSOR, destroy_cursor_traits> scoped_cursor;
typedef scoped_object_gdi<HICON, destroy_icon_traits> scoped_icon;

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_OBJECT_GDI_H_