/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_GDI_OBJECT_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_GDI_OBJECT_H_

#include <windows.h>

namespace traa {
namespace base {

template <class T, class Traits> class scoped_gdi_object {
public:
  scoped_gdi_object() : handle_(NULL) {}
  explicit scoped_gdi_object(T object) : handle_(object) {}

  ~scoped_gdi_object() { Traits::close(handle_); }

  scoped_gdi_object(const scoped_gdi_object &) = delete;
  scoped_gdi_object &operator=(const scoped_gdi_object &) = delete;

  T get() { return handle_; }

  void set(T object) {
    if (handle_ && object != handle_)
      Traits::close(handle_);
    handle_ = object;
  }

  scoped_gdi_object &operator=(T object) {
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

class recovery_thread_dpi_awareness_traits {
public:
  recovery_thread_dpi_awareness_traits() = delete;
  recovery_thread_dpi_awareness_traits(const recovery_thread_dpi_awareness_traits &) = delete;
  recovery_thread_dpi_awareness_traits &
  operator=(const recovery_thread_dpi_awareness_traits &) = delete;

  // Closes the handle.
  static void close(DPI_AWARENESS_CONTEXT context) {
    if (context)
      ::SetThreadDpiAwarenessContext(context);
  }
};

using scoped_bitmap_t = scoped_gdi_object<HBITMAP, delete_object_traits<HBITMAP>>;
using scoped_cursor_t = scoped_gdi_object<HCURSOR, destroy_cursor_traits>;
using scoped_icon_t = scoped_gdi_object<HICON, destroy_icon_traits>;
using scoped_dpi_awareness_context_t =
    scoped_gdi_object<DPI_AWARENESS_CONTEXT, recovery_thread_dpi_awareness_traits>;

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_SCOPED_GDI_OBJECT_H_