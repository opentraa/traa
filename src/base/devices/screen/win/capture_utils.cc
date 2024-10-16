#include "base/devices/screen/win/capture_utils.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/utils.h"
#include "base/log/logger.h"
#include "base/utils/win/version.h"

#include <libyuv/scale_argb.h>

#include <dwmapi.h>
#include <mutex>
#include <stdio.h>

namespace traa {
namespace base {

// capture utils

void capture_utils::dump_bmp(const uint8_t *data, const traa_size &size, const char *file_name) {
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

bool capture_utils::is_window_response(HWND window) {
  if (!window) {
    return false;
  }

  unsigned int timeout = 50;

  // Workaround for the issue that SendMessageTimeout may hang on Windows 10 RS5
  if (::GetForegroundWindow() == window && os_get_version() == version_alias::VERSION_WIN10_RS5) {
    timeout = 200;
  }

  return ::SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, timeout, nullptr) != 0;
}

bool capture_utils::get_window_image_by_gdi(HWND window, const traa_size &target_size,
                                            uint8_t **data, traa_size &scaled_size) {
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
  bmi.bmiHeader.biSizeImage =
      window_size.width() * window_size.height() * desktop_frame::kBytesPerPixel;

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

    const desktop_size scaled_desktop_size =
        calc_scaled_size(window_size, desktop_size(target_size.width, target_size.height));
    if (scaled_desktop_size.is_empty()) {
      LOG_ERROR("calc scaled scaled_size failed, get empty scaled_size");
      break;
    }

    *data = new uint8_t[scaled_desktop_size.width() * scaled_desktop_size.height() *
                        desktop_frame::kBytesPerPixel];
    if (!*data) {
      LOG_ERROR("alloc memory for thumbnail data failed: {}", ::GetLastError());
      break;
    }

    constexpr int bytes_per_pixel = desktop_frame::kBytesPerPixel;

    if (os_get_version() >= version_alias::VERSION_WIN8 && is_window_response(window)) {
      result = ::PrintWindow(window, compatible_dc, PW_RENDERFULLCONTENT);
      if (result) {
        if (scaled_desktop_size.equals(window_size)) {
          memcpy_s(*data,
                   scaled_desktop_size.width() * scaled_desktop_size.height() * bytes_per_pixel,
                   bitmap_data, window_size.width() * window_size.height() * bytes_per_pixel);
        } else {
          // use libyuv to scale the image
          libyuv::ARGBScale(
              bitmap_data, window_size.width() * bytes_per_pixel, window_size.width(),
              window_size.height(), *data, scaled_desktop_size.width() * bytes_per_pixel,
              scaled_desktop_size.width(), scaled_desktop_size.height(), libyuv::kFilterBox);
        }
      }
    }

    // use gdi to get the window image as the fallback method
    if (!result) {
      SetStretchBltMode(compatible_dc, COLORONCOLOR);
      result = ::StretchBlt(compatible_dc, 0, 0, scaled_desktop_size.width(),
                            scaled_desktop_size.height(), window_dc, 0, 0, window_size.width(),
                            window_size.height(), SRCCOPY | CAPTUREBLT);
      if (!result) {
        LOG_ERROR("stretch blt failed: {}", ::GetLastError());
        break;
      }

      for (int i = 0; i < scaled_desktop_size.height(); i++) {
        memcpy_s(*data + i * scaled_desktop_size.width() * bytes_per_pixel,
                 scaled_desktop_size.width() * bytes_per_pixel,
                 bitmap_data + i * window_size.width() * bytes_per_pixel,
                 scaled_desktop_size.width() * bytes_per_pixel);
      }
    }

    scaled_size = scaled_desktop_size.to_traa_size();
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

bool capture_utils::is_dwm_supported() {
#if defined(TRAA_SUPPORT_XP)
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
#else
  return true;
#endif
}

bool capture_utils::get_window_image_by_dwm(HWND dwm_window, HWND window,
                                            const traa_size &target_size, uint8_t **data,
                                            traa_size &scaled_size) {
  if (!is_dwm_supported()) {
    return false;
  }

  if (!dwm_window || !window || target_size.width <= 0 || target_size.height <= 0) {
    return false;
  }

  int dwm_window_width = 0;
  int dwm_window_height = 0;

  RECT rc;
  if (!::IsIconic(window) && ::GetWindowRect(window, &rc)) {
    dwm_window_width = rc.right - rc.left;
    dwm_window_height = rc.bottom - rc.top;
  } else {
    dwm_window_width = target_size.width;
    dwm_window_height = target_size.height;
  }

  desktop_size scaled_dwm_window_size =
      calc_scaled_size(desktop_size(dwm_window_width, dwm_window_height),
                       desktop_size(target_size.width, target_size.height));

  if (!::SetWindowPos(dwm_window, HWND_TOP, 0, 0, scaled_dwm_window_size.width(),
                      scaled_dwm_window_size.height(),
                      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER)) {
    LOG_ERROR("set window pos failed: {}", ::GetLastError());
    return false;
  }

  HTHUMBNAIL thumbnail_id = nullptr;
  if (FAILED(::DwmRegisterThumbnail(dwm_window, window, &thumbnail_id))) {
    LOG_ERROR("register thumbnail failed: {}", GetLastError());
    return false;
  }

  DWM_THUMBNAIL_PROPERTIES properties = {};
  properties.fVisible = TRUE;
  properties.fSourceClientAreaOnly = FALSE;
  properties.opacity = 180; // 255*0.7
  properties.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_SOURCECLIENTAREAONLY;
  properties.rcDestination = {0, 0, scaled_dwm_window_size.width(),
                              scaled_dwm_window_size.height()};

  if (FAILED(::DwmUpdateThumbnailProperties(thumbnail_id, &properties))) {
    LOG_ERROR("update thumbnail properties failed: {}", GetLastError());
    ::DwmUnregisterThumbnail(thumbnail_id);
    return false;
  }

  bool ret =
      get_window_image_by_gdi(dwm_window, scaled_dwm_window_size.to_traa_size(), data, scaled_size);

  if (FAILED(::DwmUnregisterThumbnail(thumbnail_id))) {
    LOG_ERROR("unregister thumbnail failed: {}", GetLastError());
  }

  return ret;
}

// thumbnail

thumbnail::thumbnail() : dwm_window_(nullptr) {
  if (!capture_utils::is_dwm_supported()) {
    LOG_INFO("dwm is not supported");
    return;
  }

  HMODULE current_module = nullptr;
  if (!::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<char *>(&DefWindowProc), &current_module)) {
    LOG_ERROR("get current module failed: {}", ::GetLastError());
    return;
  }

  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.lpfnWndProc = &DefWindowProc;
  wcex.hInstance = current_module;
  wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
  wcex.lpszClassName = L"traa_thumbnail_host";

  if (::RegisterClassExW(&wcex) == 0) {
    LOG_ERROR("register class failed: {}", ::GetLastError());
    return;
  }

  dwm_window_ = ::CreateWindowExW(WS_EX_LAYERED, L"traa_thumbnail_host", L"traa_thumbnail_host",
                                  WS_POPUP | WS_VISIBLE, 0, 0, 1, 1, nullptr, nullptr,
                                  current_module, nullptr);
  if (dwm_window_ == nullptr) {
    LOG_ERROR("create window failed: {}", ::GetLastError());
    return;
  }

  ::ShowWindow(dwm_window_, SW_HIDE);
}

thumbnail::~thumbnail() {
  if (dwm_window_ != nullptr) {
    ::DestroyWindow(dwm_window_);
  }
}

bool thumbnail::get_thumbnail_data(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                                   traa_size &size) {
  if (!capture_utils::get_window_image_by_dwm(dwm_window_, window, thumbnail_size, data, size)) {
    return capture_utils::get_window_image_by_gdi(window, thumbnail_size, data, size);
  }

  return true;
}

} // namespace base
} // namespace traa