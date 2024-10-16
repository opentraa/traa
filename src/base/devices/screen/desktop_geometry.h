#ifndef TRAA_BASE_DEVICES_SCREEN_DESKTOP_GEOMETRY_H_
#define TRAA_BASE_DEVICES_SCREEN_DESKTOP_GEOMETRY_H_

#include <traa/base.h>

namespace traa {
namespace base {

class desktop_vector {
public:
  desktop_vector() : x_(0), y_(0) {}
  desktop_vector(int32_t x, int32_t y) : x_(x), y_(y) {}

  int32_t x() const { return x_; }
  int32_t y() const { return y_; }
  bool is_zero() const { return x_ == 0 && y_ == 0; }

  bool equals(const desktop_vector &other) const { return x_ == other.x_ && y_ == other.y_; }

  void set(int32_t x, int32_t y) {
    x_ = x;
    y_ = y;
  }

  desktop_vector add(const desktop_vector &other) const {
    return desktop_vector(x() + other.x(), y() + other.y());
  }
  desktop_vector subtract(const desktop_vector &other) const {
    return desktop_vector(x() - other.x(), y() - other.y());
  }

  desktop_vector operator-() const { return desktop_vector(-x_, -y_); }

  traa_point to_traa_point() const { return traa_point(x_, y_); }

private:
  int32_t x_;
  int32_t y_;
};

// Type used to represent screen/window size.
class desktop_size {
public:
  desktop_size() : width_(0), height_(0) {}
  desktop_size(int32_t width, int32_t height) : width_(width), height_(height) {}
  desktop_size(const traa_size &size) : width_(size.width), height_(size.height) {}

  int32_t width() const { return width_; }
  int32_t height() const { return height_; }

  bool is_empty() const { return width_ <= 0 || height_ <= 0; }

  bool equals(const desktop_size &other) const {
    return width_ == other.width_ && height_ == other.height_;
  }

  void set(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
  }

  traa_size to_traa_size() const { return traa_size(width_, height_); }

private:
  int32_t width_;
  int32_t height_;
};

// Represents a rectangle on the screen.
class desktop_rect {
public:
  static desktop_rect make_size(const desktop_size &size) {
    return desktop_rect(0, 0, size.width(), size.height());
  }
  static desktop_rect make_wh(int32_t width, int32_t height) {
    return desktop_rect(0, 0, width, height);
  }
  static desktop_rect make_xywh(int32_t x, int32_t y, int32_t width, int32_t height) {
    return desktop_rect(x, y, x + width, y + height);
  }
  static desktop_rect make_ltrb(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    return desktop_rect(left, top, right, bottom);
  }
  static desktop_rect make_origin_size(const desktop_vector &origin, const desktop_size &size) {
    return make_xywh(origin.x(), origin.y(), size.width(), size.height());
  }

  desktop_rect() : left_(0), top_(0), right_(0), bottom_(0) {}

  int32_t left() const { return left_; }
  int32_t top() const { return top_; }
  int32_t right() const { return right_; }
  int32_t bottom() const { return bottom_; }
  int32_t width() const { return right_ - left_; }
  int32_t height() const { return bottom_ - top_; }

  void set_width(int32_t width) { right_ = left_ + width; }
  void set_height(int32_t height) { bottom_ = top_ + height; }

  desktop_vector top_left() const { return desktop_vector(left_, top_); }
  desktop_size size() const { return desktop_size(width(), height()); }

  bool is_empty() const { return left_ >= right_ || top_ >= bottom_; }

  bool equals(const desktop_rect &other) const {
    return left_ == other.left_ && top_ == other.top_ && right_ == other.right_ &&
           bottom_ == other.bottom_;
  }

  // Returns true if `point` lies within the rectangle boundaries.
  bool contains(const desktop_vector &point) const;

  // Returns true if `rect` lies within the boundaries of this rectangle.
  bool contains(const desktop_rect &rect) const;

  // Finds intersection with `rect`.
  void intersect_width(const desktop_rect &rect);

  // Extends the rectangle to cover `rect`. If `this` is empty, replaces `this`
  // with `rect`; if `rect` is empty, this function takes no effect.
  void union_with(const desktop_rect &rect);

  // Adds (dx, dy) to the position of the rectangle.
  void translate(int32_t dx, int32_t dy);
  void translate(desktop_vector d) { translate(d.x(), d.y()); }

  // Enlarges current desktop_rect by subtracting `left_offset` and `top_offset`
  // from `left_` and `top_`, and adding `right_offset` and `bottom_offset` to
  // `right_` and `bottom_`. This function does not normalize the result, so
  // `left_` and `top_` may be less than zero or larger than `right_` and
  // `bottom_`.
  void extend(int32_t left_offset, int32_t top_offset, int32_t right_offset, int32_t bottom_offset);

  // Scales current desktop_rect. This function does not impact the `top_` and
  // `left_`.
  void scale(double horizontal, double vertical);

  traa_rect to_traa_rect() const { return traa_rect(left_, top_, right_, bottom_); }

private:
  desktop_rect(int32_t left, int32_t top, int32_t right, int32_t bottom)
      : left_(left), top_(top), right_(right), bottom_(bottom) {}

  int32_t left_;
  int32_t top_;
  int32_t right_;
  int32_t bottom_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_DESKTOP_GEOMETRY_H_