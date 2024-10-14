#include "base/devices/screen/utils.h"
#include "base/devices/screen/desktop_frame.h"

#include <cmath>

#if defined(_WIN32)
#include <Windows.h>
#include <stdio.h>
#endif // _WIN32

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

#if defined(_WIN32)
void dump_bmp(const uint8_t *data, const traa_size &size, const char *file_name) {
  if (!data || size.width <= 0 || size.height <= 0) {
    return;
  }

  FILE *file = fopen(file_name, "wb+");
  if (!file) {
    return;
  }

  BITMAPFILEHEADER file_header = {};
  file_header.bfType = 0x4D42;
  file_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
                       size.width * size.height * desktop_frame::kBytesPerPixel;
  file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  BITMAPINFOHEADER info_header = {};
  info_header.biSize = sizeof(BITMAPINFOHEADER);
  info_header.biWidth = size.width;
  info_header.biHeight = -size.height;
  info_header.biPlanes = 1;
  info_header.biBitCount = 32;
  info_header.biCompression = BI_RGB;
  info_header.biSizeImage = size.width * size.height * desktop_frame::kBytesPerPixel;

  fwrite(&file_header, sizeof(BITMAPFILEHEADER), 1, file);
  fwrite(&info_header, sizeof(BITMAPINFOHEADER), 1, file);
  fwrite(data, size.width * size.height * desktop_frame::kBytesPerPixel, 1, file);

  fclose(file);
}
#endif // _WIN32

} // namespace base
} // namespace traa