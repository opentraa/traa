#include "base/devices/screen/utils.h"

#include <cmath>

namespace traa {
namespace base {

desktop_size calc_scaled_size(const desktop_size &source, const desktop_size &dest) {
  if (source.width() == 0 || source.height() == 0 || dest.width() == 0 || dest.height() == 0) {
    return desktop_size(0, 0);
  }

  auto src_image_size = source.width() * source.height();
  auto dst_image_size = dest.width() * dest.height();
  if (src_image_size <= dst_image_size) {
    return source;
  }

  // Calculate the scale factor to fit the source image into the destination image.
  double scale_factor = std::sqrt(static_cast<double>(dst_image_size) / src_image_size);

  // The codec requires the width and height to be multiples of 2.
  int32_t scaled_width = static_cast<int32_t>(source.width() * scale_factor) & 0xFFFFFFFE;
  int32_t scaled_height = static_cast<int32_t>(source.height() * scale_factor) & 0xFFFFFFFE;

  return desktop_size(scaled_width, scaled_height);
}

} // namespace base
} // namespace traa