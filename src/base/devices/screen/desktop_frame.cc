#include "desktop_frame.h"

#include <libyuv.h>

#include <cmath>
#include <cstring>

namespace traa {
namespace base {

desktop_frame::desktop_frame(desktop_size size, int stride, uint8_t *data,
                             shared_memory *shared_memory)
    : data_(data), shared_memory_(shared_memory), size_(size), stride_(stride), capture_time_ms_(0),
      capturer_id_(traa_screen_capturer_id::TRAA_SCREEN_CAPTURER_ID_UNKNOWN) {}

desktop_frame::~desktop_frame() = default;

void desktop_frame::copy_pixels_from(const uint8_t *src_buffer, int src_stride,
                                     const desktop_rect &dest_rect) {
  uint8_t *dest = get_frame_data_at_pos(dest_rect.top_left());
  libyuv::CopyPlane(src_buffer, src_stride, dest, stride(),
                    desktop_frame::kBytesPerPixel * dest_rect.width(), dest_rect.height());
}

void desktop_frame::copy_pixels_from(const desktop_frame &src_frame, const desktop_vector &src_pos,
                                     const desktop_rect &dest_rect) {
  copy_pixels_from(src_frame.get_frame_data_at_pos(src_pos), src_frame.stride(), dest_rect);
}

bool desktop_frame::copy_intersecting_pixels_from(const desktop_frame &src_frame,
                                                  double horizontal_scale, double vertical_scale) {
  const desktop_vector &origin = top_left();
  const desktop_vector &src_frame_origin = src_frame.top_left();

  desktop_vector src_frame_offset = src_frame_origin.subtract(origin);

  // Determine the intersection, first adjusting its origin to account for any
  // DPI scaling.
  desktop_rect intersection_rect = src_frame.rect();
  if (horizontal_scale != 1.0 || vertical_scale != 1.0) {
    desktop_vector origin_adjustment(
        static_cast<int>(std::round((horizontal_scale - 1.0) * src_frame_offset.x())),
        static_cast<int>(std::round((vertical_scale - 1.0) * src_frame_offset.y())));

    intersection_rect.translate(origin_adjustment);

    src_frame_offset = src_frame_offset.add(origin_adjustment);
  }

  intersection_rect.intersect_width(rect());
  if (intersection_rect.is_empty()) {
    return false;
  }

  // Translate the intersection rect to be relative to the outer rect.
  intersection_rect.translate(-origin.x(), -origin.y());

  // Determine source position for the copy (offsets of outer frame from
  // source origin, if positive).
  int32_t src_pos_x = std::max(0, -src_frame_offset.x());
  int32_t src_pos_y = std::max(0, -src_frame_offset.y());

  copy_pixels_from(src_frame, desktop_vector(src_pos_x, src_pos_y), intersection_rect);
  return true;
}

desktop_rect desktop_frame::rect() const {
  const float scale = scale_factor();
  // Only scale the size.
  return desktop_rect::make_xywh(top_left().x(), top_left().y(),
                                 static_cast<int32_t>(size().width() / scale),
                                 static_cast<int32_t>(size().height() / scale));
}

float desktop_frame::scale_factor() const {
  float scale = 1.0f;

#if defined(TRAA_OS_MAC) || defined(CHROMEOS)
  const float kStandardDPI = 96.0f;
  // At least on Windows the logical and physical pixel are the same
  // See http://crbug.com/948362.
  if (!dpi().is_zero() && dpi().x() == dpi().y())
    scale = dpi().x() / kStandardDPI;
#endif

  return scale;
}

uint8_t *desktop_frame::get_frame_data_at_pos(const desktop_vector &pos) const {
  return data() + stride() * pos.y() + desktop_frame::kBytesPerPixel * pos.x();
}

void desktop_frame::copy_frame_info_from(const desktop_frame &other) {
  set_dpi(other.dpi());
  set_capture_time_ms(other.capture_time_ms());
  set_capturer_id(other.capturer_id());
  *mutable_updated_region() = other.updated_region();
  set_top_left(other.top_left());
  set_icc_profile(other.icc_profile());
  set_may_contain_cursor(other.may_contain_cursor());
}

void desktop_frame::move_frame_info_from(desktop_frame *other) {
  set_dpi(other->dpi());
  set_capture_time_ms(other->capture_time_ms());
  set_capturer_id(other->capturer_id());
  mutable_updated_region()->swap(other->mutable_updated_region());
  set_top_left(other->top_left());
  set_icc_profile(other->icc_profile());
  set_may_contain_cursor(other->may_contain_cursor());
}

bool desktop_frame::frame_data_is_black() const {
  if (size().is_empty())
    return false;

  uint32_t *pixel = reinterpret_cast<uint32_t *>(data());
  for (int i = 0; i < size().width() * size().height(); ++i) {
    if (*pixel++)
      return false;
  }
  return true;
}

void desktop_frame::set_frame_data_to_black() {
  const uint8_t kBlackPixelValue = 0x00;
  memset(data(), kBlackPixelValue, stride() * size().height());
}

basic_desktop_frame::basic_desktop_frame(desktop_size size)
    : desktop_frame(size, kBytesPerPixel * size.width(),
                    new uint8_t[kBytesPerPixel * size.width() * size.height()](), nullptr) {}

basic_desktop_frame::~basic_desktop_frame() { delete[] data_; }

// static
desktop_frame *basic_desktop_frame::copy_of(const desktop_frame &frame) {
  desktop_frame *result = new basic_desktop_frame(frame.size());
  // TODO(crbug.com/1330019): Temporary workaround for a known libyuv crash when
  // the height or width is 0. Remove this once this change has been merged.
  if (frame.size().width() && frame.size().height()) {
    libyuv::CopyPlane(frame.data(), frame.stride(), result->data(), result->stride(),
                      frame.size().width() * kBytesPerPixel, frame.size().height());
  }
  result->copy_frame_info_from(frame);
  return result;
}

} // namespace base
} // namespace traa
