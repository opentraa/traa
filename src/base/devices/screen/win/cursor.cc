#include "base/devices/screen/win/cursor.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/mouse_cursor.h"
#include "base/devices/screen/win/scoped_object_gdi.h"
#include "base/log/logger.h"

namespace traa {
namespace base {

namespace {

#if defined(ARCH_LITTLE_ENDIAN)

#define RGBA(r, g, b, a)                                                                           \
  ((((a) << 24) & 0xff000000) | (((b) << 16) & 0xff0000) | (((g) << 8) & 0xff00) | ((r) & 0xff))

#else // !defined(ARCH_LITTLE_ENDIAN)

#define RGBA(r, g, b, a)                                                                           \
  ((((r) << 24) & 0xff000000) | (((g) << 16) & 0xff0000) | (((b) << 8) & 0xff00) | ((a) & 0xff))

#endif // !defined(ARCH_LITTLE_ENDIAN)

constexpr int bytes_per_pixel = desktop_frame::bytes_per_pixel;

// Pixel colors used when generating cursor outlines.
constexpr uint32_t kPixelRgbaBlack = RGBA(0, 0, 0, 0xff);
constexpr uint32_t kPixelRgbaWhite = RGBA(0xff, 0xff, 0xff, 0xff);
constexpr uint32_t kPixelRgbaTransparent = RGBA(0, 0, 0, 0);

constexpr uint32_t kPixelRgbWhite = RGB(0xff, 0xff, 0xff);

// Expands the cursor shape to add a white outline for visibility against
// dark backgrounds.
void add_cursor_outline(int width, int height, uint32_t *data) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // If this is a transparent pixel (bgr == 0 and alpha = 0), check the
      // neighbor pixels to see if this should be changed to an outline pixel.
      if (*data == kPixelRgbaTransparent) {
        // Change to white pixel if any neighbors (top, bottom, left, right)
        // are black.
        if ((y > 0 && data[-width] == kPixelRgbaBlack) ||
            (y < height - 1 && data[width] == kPixelRgbaBlack) ||
            (x > 0 && data[-1] == kPixelRgbaBlack) ||
            (x < width - 1 && data[1] == kPixelRgbaBlack)) {
          *data = kPixelRgbaWhite;
        }
      }
      data++;
    }
  }
}

// Premultiplies RGB components of the pixel data in the given image by
// the corresponding alpha components.
void alpha_mul(uint32_t *data, int width, int height) {
  static_assert(sizeof(uint32_t) == bytes_per_pixel,
                "size of uint32 should be the number of bytes per pixel");

  for (uint32_t *data_end = data + width * height; data != data_end; ++data) {
    RGBQUAD *from = reinterpret_cast<RGBQUAD *>(data);
    RGBQUAD *to = reinterpret_cast<RGBQUAD *>(data);
    to->rgbBlue = (static_cast<uint16_t>(from->rgbBlue) * from->rgbReserved) / 0xff;
    to->rgbGreen = (static_cast<uint16_t>(from->rgbGreen) * from->rgbReserved) / 0xff;
    to->rgbRed = (static_cast<uint16_t>(from->rgbRed) * from->rgbReserved) / 0xff;
  }
}

// Scans a 32bpp bitmap looking for any pixels with non-zero alpha component.
// Returns true if non-zero alpha is found. `stride` is expressed in pixels.
bool has_alpha_channel(const uint32_t *data, int stride, int width, int height) {
  const RGBQUAD *plane = reinterpret_cast<const RGBQUAD *>(data);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (plane->rgbReserved != 0)
        return true;
      plane += 1;
    }
    plane += stride - width;
  }

  return false;
}

} // namespace

mouse_cursor *create_mouse_cursor_from_handle(HDC dc, HCURSOR cursor) {
  ICONINFO iinfo;
  if (!::GetIconInfo(cursor, &iinfo)) {
    LOG_ERROR("unable to get cursor icon info. last error: {}", ::GetLastError());
    return NULL;
  }

  int hotspot_x = iinfo.xHotspot;
  int hotspot_y = iinfo.yHotspot;

  // Make sure the bitmaps will be freed.
  scoped_bitmap scoped_mask(iinfo.hbmMask);
  scoped_bitmap scoped_color(iinfo.hbmColor);
  bool is_color = iinfo.hbmColor != NULL;

  // Get `scoped_mask` dimensions.
  BITMAP bitmap_info;
  if (!::GetObjectW(scoped_mask, sizeof(bitmap_info), &bitmap_info)) {
    LOG_ERROR("unable to get bitmap info. last error : {}", ::GetLastError());
    return NULL;
  }

  int width = bitmap_info.bmWidth;
  int height = bitmap_info.bmHeight;
  std::unique_ptr<uint32_t[]> mask_data(new uint32_t[width * height]);

  // Get pixel data from `scoped_mask` converting it to 32bpp along the way.
  // GetDIBits() sets the alpha component of every pixel to 0.
  BITMAPV5HEADER bmi = {0};
  bmi.bV5Size = sizeof(bmi);
  bmi.bV5Width = width;
  bmi.bV5Height = -height; // request a top-down bitmap.
  bmi.bV5Planes = 1;
  bmi.bV5BitCount = bytes_per_pixel * 8;
  bmi.bV5Compression = BI_RGB;
  bmi.bV5AlphaMask = 0xff000000;
  bmi.bV5CSType = LCS_WINDOWS_COLOR_SPACE;
  bmi.bV5Intent = LCS_GM_BUSINESS;
  if (!::GetDIBits(dc, scoped_mask, 0, height, mask_data.get(),
                   reinterpret_cast<BITMAPINFO *>(&bmi), DIB_RGB_COLORS)) {
    LOG_ERROR("unable to get bitmap bits. last error: {}", ::GetLastError());
    return NULL;
  }

  uint32_t *mask_plane = mask_data.get();
  std::unique_ptr<desktop_frame> image(new basic_desktop_frame(desktop_size(width, height)));
  bool has_alpha = false;

  if (is_color) {
    image.reset(new basic_desktop_frame(desktop_size(width, height)));
    // Get the pixels from the color bitmap.
    if (!::GetDIBits(dc, scoped_color, 0, height, image->data(),
                     reinterpret_cast<BITMAPINFO *>(&bmi), DIB_RGB_COLORS)) {
      LOG_ERROR("Unable to get bitmap bits. last error: {} ", ::GetLastError());
      return NULL;
    }

    // GetDIBits() does not provide any indication whether the bitmap has alpha
    // channel, so we use has_alpha_channel() below to find it out.
    has_alpha =
        has_alpha_channel(reinterpret_cast<uint32_t *>(image->data()), width, width, height);
  } else {
    // For non-color cursors, the mask contains both an AND and an XOR mask and
    // the height includes both. Thus, the width is correct, but we need to
    // divide by 2 to get the correct mask height.
    height /= 2;

    image.reset(new basic_desktop_frame(desktop_size(width, height)));

    // The XOR mask becomes the color bitmap.
    memcpy(image->data(), mask_plane + (width * height), image->stride() * height);
  }

  // Reconstruct transparency from the mask if the color image does not has
  // alpha channel.
  if (!has_alpha) {
    bool add_outline = false;
    uint32_t *dst = reinterpret_cast<uint32_t *>(image->data());
    uint32_t *mask = mask_plane;
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        // The two bitmaps combine as follows:
        //  mask  color   Windows Result   Our result    RGB   Alpha
        //   0     00      Black            Black         00    ff
        //   0     ff      White            White         ff    ff
        //   1     00      Screen           Transparent   00    00
        //   1     ff      Reverse-screen   Black         00    ff
        //
        // Since we don't support XOR cursors, we replace the "Reverse Screen"
        // with black. In this case, we also add an outline around the cursor
        // so that it is visible against a dark background.
        if (*mask == kPixelRgbWhite) {
          if (*dst != 0) {
            add_outline = true;
            *dst = kPixelRgbaBlack;
          } else {
            *dst = kPixelRgbaTransparent;
          }
        } else {
          *dst = kPixelRgbaBlack ^ *dst;
        }

        ++dst;
        ++mask;
      }
    }
    if (add_outline) {
      add_cursor_outline(width, height, reinterpret_cast<uint32_t *>(image->data()));
    }
  }

  // Pre-multiply the resulting pixels since mouse_cursor uses premultiplied
  // images.
  alpha_mul(reinterpret_cast<uint32_t *>(image->data()), width, height);

  return new mouse_cursor(image.release(), desktop_vector(hotspot_x, hotspot_y));
}

} // namespace base
} // namespace traa
