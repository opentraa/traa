#include "base/devices/screen/win/thumbnail.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/utils.h"
#include "base/log/logger.h"
#include "base/utils/win/version.h"

#include <libyuv/scale_argb.h>

#include <mutex>

namespace traa {
namespace base {

bool get_thumbnail_data_from_gdi(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                                 traa_size &size) {
  RECT rect;
  if (!::GetWindowRect(window, &rect)) {
    LOG_ERROR("get window rect failed: {}", ::GetLastError());
    return false;
  }

  HDC window_dc = ::GetWindowDC(window);
  if (!window_dc) {
    LOG_ERROR("get window dc failed: {}", ::GetLastError());
    return false;
  }

  desktop_size window_size(rect.right - rect.left, rect.bottom - rect.top);

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biHeight = -window_size.height();
  bmi.bmiHeader.biWidth = window_size.width();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biSizeImage = window_size.width() * window_size.height() * 4;

  bool result = false;
  HANDLE section = nullptr;
  uint8_t *bitmap_data = nullptr;
  HBITMAP bitmap = nullptr;
  HDC compatible_dc = nullptr;
  HGDIOBJ old_obj = nullptr;
  do {
    bitmap = ::CreateDIBSection(window_dc, &bmi, DIB_RGB_COLORS, (void **)&bitmap_data, section, 0);
    if (!bitmap) {
      LOG_ERROR("create dib section failed: {}", ::GetLastError());
      break;
    }

    compatible_dc = ::CreateCompatibleDC(window_dc);
    old_obj = ::SelectObject(compatible_dc, bitmap);
    if (!old_obj || old_obj == HGDI_ERROR) {
      LOG_ERROR("select object failed: {}", ::GetLastError());
      break;
    }

    const desktop_size scaled_size =
        calc_scaled_size(window_size, desktop_size(thumbnail_size.width, thumbnail_size.height));
    if (scaled_size.is_empty()) {
      LOG_ERROR("calc scaled size failed, get empty size");
      break;
    }

    *data = new uint8_t[scaled_size.width() * scaled_size.height() * 4];
    if (!*data) {
      LOG_ERROR("alloc memory for thumbnail data failed: {}", ::GetLastError());
      break;
    }

    constexpr int bytes_per_pixel = desktop_frame::kBytesPerPixel;

    if (os_get_version() >= version_alias::VERSION_WIN8) {
      result = ::PrintWindow(window, compatible_dc, 2);
      if (result) {
        libyuv::ARGBScale(bitmap_data, window_size.width() * bytes_per_pixel, window_size.width(),
                          window_size.height(), *data, scaled_size.width() * bytes_per_pixel,
                          scaled_size.width(), scaled_size.height(), libyuv::kFilterBox);
      }
    }

    // use gdi to get the window image as the fallback method
    if (!result) {
      SetStretchBltMode(compatible_dc, COLORONCOLOR);
      result =
          ::StretchBlt(compatible_dc, 0, 0, scaled_size.width(), scaled_size.height(), window_dc, 0,
                       0, window_size.width(), window_size.height(), SRCCOPY | CAPTUREBLT);
      if (!result) {
        LOG_ERROR("stretch blt failed: {}", ::GetLastError());
        break;
      }

      for (int i = 0; i < scaled_size.height(); i++) {
        memcpy_s(*data + i * scaled_size.width() * bytes_per_pixel,
                 scaled_size.width() * bytes_per_pixel,
                 bitmap_data + i * window_size.width() * bytes_per_pixel,
                 scaled_size.width() * bytes_per_pixel);
      }
    }

    size = scaled_size.to_traa_size();
  } while (0);

  if (bitmap) {
    ::DeleteObject(bitmap);
  }

  if (compatible_dc) {
    ::SelectObject(compatible_dc, old_obj);
    ::DeleteDC(compatible_dc);
  }

  ::ReleaseDC(window, window_dc);

  if (!result && *data) {
    delete[] * data;
    *data = nullptr;
  }

  return result;
}

bool is_dwm_supported() {
  static std::once_flag _flag;
  static std::atomic<bool> _supported(false);

  std::call_once(_flag, [&]() {
    HINSTANCE dwmapi = ::LoadLibraryW(L"dwmapi.dll");
    if (dwmapi != nullptr) {
      _supported.store(true, std::memory_order_release);
      ::FreeLibrary(dwmapi);
    }
  });

  return _supported.load(std::memory_order_acquire);
}

bool get_thubmnail_data_from_dwm(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                                 traa_size &size) {
  if (!is_dwm_supported()) {
    return false;
  }
  return false;
}

bool get_thumbnail_data(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                        traa_size &size) {
  if (!get_thubmnail_data_from_dwm(window, thumbnail_size, data, size)) {
    return get_thumbnail_data_from_gdi(window, thumbnail_size, data, size);
  }

  return true;
}

} // namespace base
} // namespace traa