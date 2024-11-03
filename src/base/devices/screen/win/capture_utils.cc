#include "base/devices/screen/win/capture_utils.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/utils.h"
#include "base/devices/screen/win/scoped_object_gdi.h"
#include "base/log/logger.h"
#include "base/utils/win/version.h"

#include <libyuv/scale_argb.h>

#include <dwmapi.h>
#include <mutex>
#include <stdio.h>

#include <shellscalingapi.h>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

// dpi

float capture_utils::get_dpi_scale(HWND window) {
  const int dpi = ::GetDpiForWindow(window);
  return static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
}

bool capture_utils::is_dpi_aware() {
  if (os_get_version() < version_alias::VERSION_WIN8) {
    return true;
  }

  PROCESS_DPI_AWARENESS dpi_aware = PROCESS_DPI_UNAWARE;
  if (SUCCEEDED(::GetProcessDpiAwareness(nullptr, &dpi_aware))) {
    return dpi_aware != PROCESS_DPI_UNAWARE;
  }

  return false;
}

bool capture_utils::is_dpi_aware(HWND window) {
  if (!window) {
    return is_dpi_aware();
  }

  const int dpi = ::GetDpiForWindow(window);
  return dpi != USER_DEFAULT_SCREEN_DPI;
}

// gdi

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
                       size.width * size.height * desktop_frame::bytes_per_pixel;
  file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  BITMAPINFOHEADER info_header = {};
  info_header.biSize = sizeof(BITMAPINFOHEADER);
  info_header.biWidth = size.width;
  info_header.biHeight = -size.height;
  info_header.biPlanes = 1;
  info_header.biBitCount = 32;
  info_header.biCompression = BI_RGB;
  info_header.biSizeImage = size.width * size.height * desktop_frame::bytes_per_pixel;

  fwrite(&file_header, sizeof(BITMAPFILEHEADER), 1, file);
  fwrite(&info_header, sizeof(BITMAPINFOHEADER), 1, file);
  fwrite(data, size.width * size.height * desktop_frame::bytes_per_pixel, 1, file);

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

bool capture_utils::is_window_maximized(HWND window, bool *result) {
  WINDOWPLACEMENT placement;
  ::memset(&placement, 0, sizeof(WINDOWPLACEMENT));
  placement.length = sizeof(WINDOWPLACEMENT);
  if (!::GetWindowPlacement(window, &placement)) {
    return false;
  }

  *result = (placement.showCmd == SW_SHOWMAXIMIZED);
  return true;
}

bool capture_utils::is_window_owned_by_current_process(HWND window) {
  DWORD process_id;
  ::GetWindowThreadProcessId(window, &process_id);
  return process_id == ::GetCurrentProcessId();
}

bool capture_utils::get_window_rect(HWND window, desktop_rect *rect) {
  RECT rc;
  if (!::GetWindowRect(window, &rc)) {
    LOG_ERROR("get window rect failed: {}", ::GetLastError());
    return false;
  }

  *rect = desktop_rect::make_ltrb(rc.left, rc.top, rc.right, rc.bottom);

  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowrect
  // GetWindowRect is virtualized for DPI.
  //
  // https://stackoverflow.com/questions/8060280/getting-an-dpi-aware-correct-rect-from-getwindowrect-from-a-external-window
  // if the window is not owned by the current process, and the current process is not DPI aware, we
  // need to scale the rect.
  //
  // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setthreaddpiawarenesscontext
  // should we need to consider the case that the window is owned by the current process, but the
  // aware context is not the same with current thread? they may use SetThreadDpiAwarenessContext to
  // change the aware context after windows 10 1607.
  //
  // https://github.com/obsproject/obs-studio/issues/8706
  // https://github.com/obsproject/obs-studio/blob/e9ef38e3d38e08bffcabbb59230b94baa41ede96/plugins/win-capture/window-capture.c#L604-L612
  // obs also faced this issue, and they use SetThreadDpiAwarenessContext to change current thread's
  // aware context to the same with the window.
  //
  // WTF!!!!!
  //
  // howerver, the dpi awareness context is set to aware for most of the time, so we do not need to
  // consider this too much for now.
  //
#if 0 // !!!!!!!!!!!!!!!!!!!!!!! THIS DO NOT TAKE ANY EFFECT !!!!!!!!!!!!!!!!
  std::unique_ptr<scoped_dpi_awareness_context> pre_awareness_context;
  if (os_get_version() >= VERSION_WIN10_RS1) {
    const DPI_AWARENESS_CONTEXT context = ::GetWindowDpiAwarenessContext(window);
    pre_awareness_context.reset(
        new scoped_dpi_awareness_context(::SetThreadDpiAwarenessContext(context)));
  } else {
#endif
  if (!is_window_owned_by_current_process(window) && !is_dpi_aware()) {
    const float scale_factor = get_dpi_scale(window);
    rect->scale(scale_factor, scale_factor);
  }
#if 0
  }
#endif

  return true;
}

bool capture_utils::get_window_cropped_rect(HWND window, bool avoid_cropping_border,
                                            desktop_rect *cropped_rect,
                                            desktop_rect *original_rect) {
  if (!get_window_rect(window, original_rect)) {
    return false;
  }

  *cropped_rect = *original_rect;

  bool is_maximized = false;
  if (!is_window_maximized(window, &is_maximized)) {
    return false;
  }

  // As of Windows8, transparent resize borders are added by the OS at
  // left/bottom/right sides of a resizeable window. If the cropped window
  // doesn't remove these borders, the background will be exposed a bit.
  if (os_get_version() >= VERSION_WIN8 || is_maximized) {
    // Only apply this cropping to windows with a resize border (otherwise,
    // it'd clip the edges of captured pop-up windows without this border).
    RECT rect;
    DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
    // it's means that the window edge is not transparent
    if (original_rect && rect.left == original_rect->left()) {
      return true;
    }
    LONG style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_THICKFRAME || style & DS_MODALFRAME) {
      int width = GetSystemMetrics(SM_CXSIZEFRAME);
      int bottom_height = GetSystemMetrics(SM_CYSIZEFRAME);
      const int visible_border_height = GetSystemMetrics(SM_CYBORDER);
      int top_height = visible_border_height;

      // If requested, avoid cropping the visible window border. This is used
      // for pop-up windows to include their border, but not for the outermost
      // window (where a partially-transparent border may expose the
      // background a bit).
      if (avoid_cropping_border) {
        width = std::max(0, width - GetSystemMetrics(SM_CXBORDER));
        bottom_height = std::max(0, bottom_height - visible_border_height);
        top_height = 0;
      }
      cropped_rect->extend(-width, -top_height, -width, -bottom_height);
    }
  }

  return true;
}

bool capture_utils::get_window_image_by_gdi(HWND window, const traa_size &target_size,
                                            uint8_t **data, traa_size &scaled_size) {
  desktop_rect rect;
  if (!get_window_rect(window, &rect)) {
    return false;
  }

  HDC window_dc = ::GetWindowDC(window);
  if (!window_dc) {
    LOG_ERROR("get window dc failed: {}", ::GetLastError());
    return false;
  }

  bool result = false;
  HANDLE section = nullptr;
  uint8_t *bitmap_data = nullptr;
  HBITMAP bitmap = nullptr;
  HDC compatible_dc = nullptr;
  HGDIOBJ old_obj = nullptr;
  do {
    constexpr int bytes_per_pixel = desktop_frame::bytes_per_pixel;

    desktop_size window_size = rect.size();

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biHeight = -window_size.height();
    bmi.bmiHeader.biWidth = window_size.width();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biSizeImage = window_size.width() * window_size.height() * bytes_per_pixel;

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

    int32_t data_size =
        scaled_desktop_size.width() * scaled_desktop_size.height() * bytes_per_pixel;
    *data = new uint8_t[data_size];
    if (!*data) {
      LOG_ERROR("alloc memory for thumbnail data failed: {}", ::GetLastError());
      break;
    }

    if (os_get_version() >= version_alias::VERSION_WIN8 && is_window_response(window)) {
      result = ::PrintWindow(window, compatible_dc, PW_RENDERFULLCONTENT);
      if (result) {
        if (scaled_desktop_size.equals(window_size)) {
          memcpy_s(*data, data_size, bitmap_data,
                   window_size.width() * window_size.height() * bytes_per_pixel);
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
    if (old_obj) {
      ::SelectObject(compatible_dc, old_obj);
    }
    ::DeleteDC(compatible_dc);
  }

  ::ReleaseDC(window, window_dc);

  if (!result && *data) {
    delete[] *data;
    *data = nullptr;
  }

  return result;
}

bool capture_utils::get_screen_image_by_gdi(const traa_rect &rect, const traa_size &target_size,
                                            uint8_t **data, traa_size &scaled_size) {
  const desktop_size scaled_desktop_size =
      calc_scaled_size(desktop_size(rect.right - rect.left, rect.bottom - rect.top),
                       desktop_size(target_size.width, target_size.height));
  if (scaled_desktop_size.is_empty()) {
    LOG_ERROR("calc scaled scaled_size failed, get empty scaled_size");
    return false;
  }

  HDC screen_dc = ::GetDC(nullptr);
  if (!screen_dc) {
    LOG_ERROR("get screen dc failed: {}", ::GetLastError());
    return false;
  }

  bool result = false;
  HANDLE section = nullptr;
  uint8_t *bitmap_data = nullptr;
  HBITMAP bitmap = nullptr;
  HDC compatible_dc = nullptr;
  HGDIOBJ old_obj = nullptr;

  do {
    constexpr int bytes_per_pixel = desktop_frame::bytes_per_pixel;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biWidth = scaled_desktop_size.width();
    bmi.bmiHeader.biHeight = -scaled_desktop_size.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biSizeImage =
        scaled_desktop_size.width() * scaled_desktop_size.height() * bytes_per_pixel;

    bitmap = ::CreateDIBSection(screen_dc, &bmi, DIB_RGB_COLORS, (void **)&bitmap_data, section, 0);
    if (!bitmap) {
      LOG_ERROR("create dib section failed: {}", ::GetLastError());
      break;
    }

    compatible_dc = ::CreateCompatibleDC(screen_dc);
    old_obj = ::SelectObject(compatible_dc, bitmap);
    if (!old_obj || old_obj == HGDI_ERROR) {
      LOG_ERROR("select object failed: {}", ::GetLastError());
      break;
    }

    SetStretchBltMode(compatible_dc, COLORONCOLOR);
    result = ::StretchBlt(compatible_dc, 0, 0, scaled_desktop_size.width(),
                          scaled_desktop_size.height(), screen_dc, rect.left, rect.top,
                          rect.right - rect.left, rect.bottom - rect.top, SRCCOPY | CAPTUREBLT);
    if (!result) {
      LOG_ERROR("stretch blt failed: {}", ::GetLastError());
      break;
    }

    *data = new uint8_t[bmi.bmiHeader.biSizeImage];
    if (!*data) {
      LOG_ERROR("alloc memory for thumbnail data failed: {}", ::GetLastError());
      break;
    }

    memcpy_s(*data, bmi.bmiHeader.biSizeImage, bitmap_data, bmi.bmiHeader.biSizeImage);

    scaled_size = scaled_desktop_size.to_traa_size();
  } while (0);

  if (bitmap) {
    ::DeleteObject(bitmap);
  }

  if (compatible_dc) {
    if (old_obj) {
      ::SelectObject(compatible_dc, old_obj);
    }
    ::DeleteDC(compatible_dc);
  }

  ::ReleaseDC(nullptr, screen_dc);

  if (!result && *data) {
    delete[] *data;
    *data = nullptr;
  }

  return true;
}

bool capture_utils::is_dwm_composition_enabled() {
  BOOL enabled = FALSE;
  if (SUCCEEDED(::DwmIsCompositionEnabled(&enabled))) {
    return enabled == TRUE;
  }
  return false;
}

bool capture_utils::is_window_cloaked(HWND window) {
  DWORD cloaked;
  HRESULT hr = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
  return SUCCEEDED(hr) && cloaked;
}

bool capture_utils::get_window_image_by_dwm(HWND dwm_window, HWND window,
                                            const traa_size &target_size, uint8_t **data,
                                            traa_size &scaled_size) {
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