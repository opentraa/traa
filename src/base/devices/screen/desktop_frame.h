/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_H_
#define TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_H_

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>
#include <vector>

namespace traa {
namespace base {

// desktop_frame represents a video frame captured from the screen.
class desktop_frame {
public:
  // desktop_frame objects always hold BGRA data.
  static constexpr int k_bytes_per_pixel = 4;

  virtual ~desktop_frame();

  desktop_frame(const desktop_frame &) = delete;
  desktop_frame &operator=(const desktop_frame &) = delete;

  // Returns the rectangle in full desktop coordinates to indicate it covers
  // the area of top_left() to top_letf() + size() / scale_factor().
  desktop_rect rect() const;

  // Returns the scale factor from DIPs to physical pixels of the frame.
  // Assumes same scale in both X and Y directions at present.
  float scale_factor() const;

  // Size of the frame. In physical coordinates, mapping directly from the
  // underlying buffer.
  const desktop_size &size() const { return size_; }

  // The top-left of the frame in full desktop coordinates. E.g. the top left
  // monitor should start from (0, 0). The desktop coordinates may be scaled by
  // OS, but this is always consistent with the MouseCursorMonitor.
  const desktop_vector &top_left() const { return top_left_; }
  void set_top_left(const desktop_vector &top_left) { top_left_ = top_left; }

  // Distance in the buffer between two neighboring rows in bytes.
  int stride() const { return stride_; }

  // Data buffer used for the frame.
  uint8_t *data() const { return data_; }

  // get_shared_memory used for the buffer or NULL if memory is allocated on the
  // heap. The result is guaranteed to be deleted only after the frame is
  // deleted (classes that inherit from desktop_frame must ensure it).
  shared_memory *get_shared_memory() const { return shared_memory_; }

  // Indicates region of the screen that has changed since the previous frame.
  const desktop_region &updated_region() const { return updated_region_; }
  desktop_region *mutable_updated_region() { return &updated_region_; }

  // DPI of the screen being captured. May be set to zero, e.g. if DPI is
  // unknown.
  const desktop_vector &dpi() const { return dpi_; }
  void set_dpi(const desktop_vector &dpi) { dpi_ = dpi; }

  // Indicates if this frame may have the mouse cursor in it. Capturers that
  // support cursor capture may set this to true. If the cursor was
  // outside of the captured area, this may be true even though the cursor is
  // not in the image.
  bool may_contain_cursor() const { return may_contain_cursor_; }
  void set_may_contain_cursor(bool may_contain_cursor) { may_contain_cursor_ = may_contain_cursor; }

  // Time taken to capture the frame in milliseconds.
  int64_t capture_time_ms() const { return capture_time_ms_; }
  void set_capture_time_ms(int64_t time_ms) { capture_time_ms_ = time_ms; }

  // Copies pixels from a buffer or another frame. `dest_rect` rect must lay
  // within bounds of this frame.
  void copy_pixels_from(const uint8_t *src_buffer, int src_stride, const desktop_rect &dest_rect);
  void copy_pixels_from(const desktop_frame &src_frame, const desktop_vector &src_pos,
                        const desktop_rect &dest_rect);

  // Copies pixels from another frame, with the copied & overwritten regions
  // representing the intersection between the two frames. Returns true if
  // pixels were copied, or false if there's no intersection. The scale factors
  // represent the ratios between pixel space & offset coordinate space (e.g.
  // 2.0 would indicate the frames are scaled down by 50% for display, so any
  // offset between their origins should be doubled).
  bool copy_intersecting_pixels_from(const desktop_frame &src_frame, double horizontal_scale,
                                     double vertical_scale);

  // A helper to return the data pointer of a frame at the specified position.
  uint8_t *get_frame_data_at_pos(const desktop_vector &pos) const;

  // The desktop_capturer implementation which generates current desktop_frame.
  // Not all desktop_capturer implementations set this field; it's set to
  // kUnknown by default.
  uint32_t get_capturer_id() const { return capturer_id_; }
  void set_capturer_id(uint32_t capturer_id) { capturer_id_ = capturer_id; }

  // Copies various information from `other`. Anything initialized in
  // constructor are not copied.
  // This function is usually used when sharing a source desktop_frame with
  // several clients: the original desktop_frame should be kept unchanged. For
  // example, basic_desktop_frame::copy_of() and SharedDesktopFrame::Share().
  void copy_frame_info_from(const desktop_frame &other);

  // Copies various information from `other`. Anything initialized in
  // constructor are not copied. Not like copy_frame_info_from() function, this
  // function uses swap or move constructor to avoid data copy. It won't break
  // the `other`, but some of its information may be missing after this
  // operation. E.g. other->updated_region_;
  // This function is usually used when wrapping a desktop_frame: the wrapper
  // instance takes the ownership of `other`, so other components cannot access
  // `other` anymore. For example, CroppedDesktopFrame and
  // DesktopFrameWithCursor.
  void move_frame_info_from(desktop_frame *other);

  // Set and get the ICC profile of the frame data pixels. Useful to build the
  // a ColorSpace object from clients of webrtc library like chromium. The
  // format of an ICC profile is defined in the following specification
  // http://www.color.org/specification/ICC1v43_2010-12.pdf.
  const std::vector<uint8_t> &icc_profile() const { return icc_profile_; }
  void set_icc_profile(const std::vector<uint8_t> &icc_profile) { icc_profile_ = icc_profile; }

  // Sets all pixel values in the data buffer to zero.
  void set_frame_data_to_black();

  // Returns true if all pixel values in the data buffer are zero or false
  // otherwise. Also returns false if the frame is empty.
  bool frame_data_is_black() const;

protected:
  desktop_frame(desktop_size size, int stride, uint8_t *data, shared_memory *shared_memory);

  // Ownership of the buffers is defined by the classes that inherit from this
  // class. They must guarantee that the buffer is not deleted before the frame
  // is deleted.
  uint8_t *const data_;
  shared_memory *const shared_memory_;

private:
  const desktop_size size_;
  const int stride_;

  desktop_region updated_region_;
  desktop_vector top_left_;
  desktop_vector dpi_;
  bool may_contain_cursor_ = false;
  int64_t capture_time_ms_;
  uint32_t capturer_id_;
  std::vector<uint8_t> icc_profile_;
};

// A desktop_frame that stores data in the heap.
class basic_desktop_frame : public desktop_frame {
public:
  // The entire data buffer used for the frame is initialized with zeros.
  explicit basic_desktop_frame(desktop_size size);

  ~basic_desktop_frame() override;

  basic_desktop_frame(const basic_desktop_frame &) = delete;
  basic_desktop_frame &operator=(const basic_desktop_frame &) = delete;

  // Creates a basic_desktop_frame that contains copy of `frame`.
  // TODO(zijiehe): Return std::unique_ptr<desktop_frame>
  static desktop_frame *copy_of(const desktop_frame &frame);
};

// A desktop_frame that stores data in shared memory.
class shared_memory_desktop_frame : public desktop_frame {
public:
  // May return nullptr if `shared_memory_factory` failed to create a
  // memory instance.
  // `shared_memory_factory` should not be nullptr.
  static std::unique_ptr<desktop_frame> create(desktop_size size,
                                               shared_memory_factory *memory_factory);

  // Takes ownership of `memory`.
  // Deprecated, use the next constructor.
  shared_memory_desktop_frame(desktop_size size, int stride, shared_memory *memory);

  // Preferred.
  shared_memory_desktop_frame(desktop_size size, int stride, std::unique_ptr<shared_memory> memory);

  ~shared_memory_desktop_frame() override;

  shared_memory_desktop_frame(const shared_memory_desktop_frame &) = delete;
  shared_memory_desktop_frame &operator=(const shared_memory_desktop_frame &) = delete;

private:
  // Avoid unexpected order of parameter evaluation.
  // Executing both std::unique_ptr<T>::operator->() and
  // std::unique_ptr<T>::release() in the member initializer list is not safe.
  // Depends on the order of parameter evaluation,
  // std::unique_ptr<T>::operator->() may trigger assertion failure if it has
  // been evaluated after std::unique_ptr<T>::release(). By using this
  // constructor, std::unique_ptr<T>::operator->() won't be involved anymore.
  shared_memory_desktop_frame(desktop_rect rect, int stride, shared_memory *memory);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DESKTOP_FRAME_H_