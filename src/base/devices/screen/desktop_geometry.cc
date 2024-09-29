#include "base/devices/screen/desktop_geometry.h"

#include <algorithm>
#include <cmath>

namespace traa {
namespace base {

bool desktop_rect::contains(const desktop_vector &point) const {
  return point.x() >= left() && point.x() < right() && point.y() >= top() && point.y() < bottom();
}

bool desktop_rect::contains(const desktop_rect &rect) const {
  return rect.left() >= left() && rect.right() <= right() && rect.top() >= top() &&
         rect.bottom() <= bottom();
}

void desktop_rect::intersect_width(const desktop_rect &rect) {
  left_ = std::max(left(), rect.left());
  top_ = std::max(top(), rect.top());
  right_ = std::min(right(), rect.right());
  bottom_ = std::min(bottom(), rect.bottom());
  if (is_empty()) {
    left_ = 0;
    top_ = 0;
    right_ = 0;
    bottom_ = 0;
  }
}

void desktop_rect::union_with(const desktop_rect &rect) {
  if (is_empty()) {
    *this = rect;
    return;
  }

  if (rect.is_empty()) {
    return;
  }

  left_ = std::min(left(), rect.left());
  top_ = std::min(top(), rect.top());
  right_ = std::max(right(), rect.right());
  bottom_ = std::max(bottom(), rect.bottom());
}

void desktop_rect::translate(int32_t dx, int32_t dy) {
  left_ += dx;
  top_ += dy;
  right_ += dx;
  bottom_ += dy;
}

void desktop_rect::extend(int32_t left_offset, int32_t top_offset, int32_t right_offset,
                          int32_t bottom_offset) {
  left_ -= left_offset;
  top_ -= top_offset;
  right_ += right_offset;
  bottom_ += bottom_offset;
}

void desktop_rect::scale(double horizontal, double vertical) {
  right_ += static_cast<int>(std::round(width() * (horizontal - 1)));
  bottom_ += static_cast<int>(std::round(height() * (vertical - 1)));
}

} // namespace base
} // namespace traa