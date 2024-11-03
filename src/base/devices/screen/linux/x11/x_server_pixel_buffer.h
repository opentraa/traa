#ifndef BASE_DEVICES_SCREEN_LINUX_X11_X_SERVER_PIXEL_BUFFER_H_
#define BASE_DEVICES_SCREEN_LINUX_X11_X_SERVER_PIXEL_BUFFER_H_

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <memory>
#include <vector>

#include "base/devices/screen/desktop_geometry.h"

namespace traa {
namespace base {

class desktop_frame;
class x_atom_cache;

// A class to allow the X server's pixel buffer to be accessed as efficiently
// as possible.
class x_server_pixel_buffer {
public:
  x_server_pixel_buffer();
  ~x_server_pixel_buffer();

  x_server_pixel_buffer(const x_server_pixel_buffer &) = delete;
  x_server_pixel_buffer &operator=(const x_server_pixel_buffer &) = delete;

  void release();

  // Allocate (or reallocate) the pixel buffer for `window`. Returns false in
  // case of an error (e.g. window doesn't exist).
  bool init(x_atom_cache *cache, Window window);

  bool is_initialized() { return window_ != 0; }

  // Returns the size of the window the buffer was initialized for.
  desktop_size window_size() { return window_rect_.size(); }

  // Returns the rectangle of the window the buffer was initialized for.
  const desktop_rect &window_rect() { return window_rect_; }

  // Returns true if the window can be found.
  bool is_window_valid() const;

  // If shared memory is being used without pixmaps, synchronize this pixel
  // buffer with the root window contents (otherwise, this is a no-op).
  // This is to avoid doing a full-screen capture for each individual
  // rectangle in the capture list, when it only needs to be done once at the
  // beginning.
  void synchronize();

  // Capture the specified rectangle and stores it in the `frame`. In the case
  // where the full-screen data is captured by synchronize(), this simply
  // returns the pointer without doing any more work. The caller must ensure
  // that `rect` is not larger than window_size().
  bool capture_rect(const desktop_rect &rect, desktop_frame *frame);

private:
  void release_shm_segment();

  void init_shm(const XWindowAttributes &attributes);
  bool init_pixmaps(int depth);

  Display *display_ = nullptr;
  Window window_ = 0;
  desktop_rect window_rect_;
  XImage *x_image_ = nullptr;
  XShmSegmentInfo *shm_segment_info_ = nullptr;
  XImage *x_shm_image_ = nullptr;
  Pixmap shm_pixmap_ = 0;
  GC shm_gc_ = nullptr;
  bool xshm_get_image_succeeded_ = false;
  std::vector<uint8_t> icc_profile_;
};

} // namespace base
} // namespace traa

#endif // BASE_DEVICES_SCREEN_LINUX_X11_X_SERVER_PIXEL_BUFFER_H_
