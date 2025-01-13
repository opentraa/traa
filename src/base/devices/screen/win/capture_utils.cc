#include "base/devices/screen/win/capture_utils.h"

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/enumerator.h"
#include "base/devices/screen/utils.h"
#include "base/devices/screen/win/scoped_gdi_object.h"
#include "base/logger.h"
#include "base/strings/string_trans.h"
#include "base/utils/win/version.h"

#include "libyuv/scale_argb.h"

#include <dwmapi.h>
#include <mutex>
#include <stdio.h>

#include <shellscalingapi.h>

#include <algorithm>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

namespace capture_utils {
// dpi

float get_dpi_scale_for_process(HANDLE process) {
  UINT dpi = ::GetSystemDpiForProcess(process);
  return static_cast<float>(dpi) / desktop_frame::k_standard_dpi;
}

float get_dpi_scale_for_process() { return get_dpi_scale_for_process(::GetCurrentProcess()); }

float get_dpi_scale(HWND window) {
  const int dpi = ::GetDpiForWindow(window);
  return static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
}

bool is_dpi_aware() {
  if (os_get_version() < version_alias::VERSION_WIN8) {
    return true;
  }

  PROCESS_DPI_AWARENESS dpi_aware = PROCESS_DPI_UNAWARE;
  if (SUCCEEDED(::GetProcessDpiAwareness(nullptr, &dpi_aware))) {
    return dpi_aware != PROCESS_DPI_UNAWARE;
  }

  return false;
}

bool is_dpi_aware(HWND window) {
  if (!window) {
    return is_dpi_aware();
  }

  const int dpi = ::GetDpiForWindow(window);
  return dpi != USER_DEFAULT_SCREEN_DPI;
}

// gdi

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
                       size.width * size.height * desktop_frame::k_bytes_per_pixel;
  file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  BITMAPINFOHEADER info_header = {};
  info_header.biSize = sizeof(BITMAPINFOHEADER);
  info_header.biWidth = size.width;
  info_header.biHeight = -size.height;
  info_header.biPlanes = 1;
  info_header.biBitCount = 32;
  info_header.biCompression = BI_RGB;
  info_header.biSizeImage = size.width * size.height * desktop_frame::k_bytes_per_pixel;

  fwrite(&file_header, sizeof(BITMAPFILEHEADER), 1, file);
  fwrite(&info_header, sizeof(BITMAPINFOHEADER), 1, file);
  fwrite(data, size.width * size.height * desktop_frame::k_bytes_per_pixel, 1, file);

  fclose(file);
}

bool is_window_response(HWND window) {
  if (!window) {
    return false;
  }

  // 50ms is chosen in case the system is under heavy load, but it's also not
  // too long to delay window enumeration considerably.
  unsigned int timeout = 50;

  // Workaround for the issue that SendMessageTimeout may hang on Windows 10 RS5
  if (::GetForegroundWindow() == window && os_get_version() == version_alias::VERSION_WIN10_RS5) {
    timeout = 200;
  }

  return ::SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, timeout, nullptr) != 0;
}

bool is_window_maximized(HWND window, bool *result) {
  WINDOWPLACEMENT placement;
  ::memset(&placement, 0, sizeof(WINDOWPLACEMENT));
  placement.length = sizeof(WINDOWPLACEMENT);
  if (!::GetWindowPlacement(window, &placement)) {
    return false;
  }

  *result = (placement.showCmd == SW_SHOWMAXIMIZED);
  return true;
}

bool is_window_valid_and_visible(HWND window) {
  return ::IsWindow(window) && ::IsWindowVisible(window) && !::IsIconic(window);
}

bool is_window_owned_by_current_process(HWND window) {
  DWORD process_id;
  ::GetWindowThreadProcessId(window, &process_id);
  return process_id == ::GetCurrentProcessId();
}

bool get_window_rect(HWND window, desktop_rect *rect) {
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
  std::unique_ptr<scoped_dpi_awareness_context_t> pre_awareness_context;
  if (os_get_version() >= VERSION_WIN10_RS1) {
    const DPI_AWARENESS_CONTEXT context = ::GetWindowDpiAwarenessContext(window);
    pre_awareness_context.reset(
        new scoped_dpi_awareness_context_t(::SetThreadDpiAwarenessContext(context)));
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

bool get_window_cropped_rect(HWND window, bool avoid_cropping_border, desktop_rect *cropped_rect,
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
    ::DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
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

bool get_window_content_rect(HWND window, desktop_rect *result) {
  if (!get_window_rect(window, result)) {
    return false;
  }

  RECT rect;
  if (!::GetClientRect(window, &rect)) {
    return false;
  }

  const int width = rect.right - rect.left;
  // The GetClientRect() is not expected to return a larger area than
  // get_window_rect().
  if (width > 0 && width < result->width()) {
    // - GetClientRect() always set the left / top of RECT to 0. So we need to
    //   estimate the border width from GetClientRect() and get_window_rect().
    // - Border width of a window varies according to the window type.
    // - GetClientRect() excludes the title bar, which should be considered as
    //   part of the content and included in the captured frame. So we always
    //   estimate the border width according to the window width.
    // - We assume a window has same border width in each side.
    // So we shrink half of the width difference from all four sides.
    const int shrink = ((width - result->width()) / 2);
    // When `shrink` is negative, desktop_rect::extend() shrinks itself.
    result->extend(shrink, 0, shrink, 0);
    // Usually this should not happen, just in case we have received a strange
    // window, which has only left and right borders.
    if (result->height() > shrink * 2) {
      result->extend(0, shrink, 0, shrink);
    }
  }

  return true;
}

int get_window_region_type_with_boundary(HWND window, desktop_rect *result) {
  scoped_gdi_object<HRGN, delete_object_traits<HRGN>> scoped_hrgn(::CreateRectRgn(0, 0, 0, 0));
  const int region_type = ::GetWindowRgn(window, scoped_hrgn.get());

  if (region_type == SIMPLEREGION) {
    RECT rect;
    ::GetRgnBox(scoped_hrgn.get(), &rect);
    *result = desktop_rect::make_ltrb(rect.left, rect.top, rect.right, rect.bottom);
  }
  return region_type;
}

bool get_dc_size(HDC hdc, desktop_size *size) {
  scoped_gdi_object<HGDIOBJ, delete_object_traits<HGDIOBJ>> scoped_hgdi(
      ::GetCurrentObject(hdc, OBJ_BITMAP));
  BITMAP bitmap;
  memset(&bitmap, 0, sizeof(BITMAP));
  if (::GetObject(scoped_hgdi.get(), sizeof(BITMAP), &bitmap) == 0) {
    return false;
  }
  size->set(bitmap.bmWidth, bitmap.bmHeight);
  return true;
}

bool get_window_image_by_gdi(HWND window, const traa_size &target_size, uint8_t **data,
                             traa_size &scaled_size) {
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
    desktop_size window_size = rect.size();

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biHeight = -window_size.height();
    bmi.bmiHeader.biWidth = window_size.width();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biSizeImage =
        window_size.width() * window_size.height() * desktop_frame::k_bytes_per_pixel;

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

    int32_t data_size = scaled_desktop_size.width() * scaled_desktop_size.height() *
                        desktop_frame::k_bytes_per_pixel;
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
                   window_size.width() * window_size.height() * desktop_frame::k_bytes_per_pixel);
        } else {
          // use libyuv to scale the image
          libyuv::ARGBScale(bitmap_data, window_size.width() * desktop_frame::k_bytes_per_pixel,
                            window_size.width(), window_size.height(), *data,
                            scaled_desktop_size.width() * desktop_frame::k_bytes_per_pixel,
                            scaled_desktop_size.width(), scaled_desktop_size.height(),
                            libyuv::kFilterBox);
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
        memcpy_s(*data + i * scaled_desktop_size.width() * desktop_frame::k_bytes_per_pixel,
                 scaled_desktop_size.width() * desktop_frame::k_bytes_per_pixel,
                 bitmap_data + i * window_size.width() * desktop_frame::k_bytes_per_pixel,
                 scaled_desktop_size.width() * desktop_frame::k_bytes_per_pixel);
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

bool get_screen_image_by_gdi(const traa_rect &rect, const traa_size &target_size, uint8_t **data,
                             traa_size &scaled_size) {
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
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biWidth = scaled_desktop_size.width();
    bmi.bmiHeader.biHeight = -scaled_desktop_size.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biSizeImage = scaled_desktop_size.width() * scaled_desktop_size.height() *
                                desktop_frame::k_bytes_per_pixel;

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

bool is_dwm_composition_enabled() {
  BOOL enabled = FALSE;
  if (SUCCEEDED(::DwmIsCompositionEnabled(&enabled))) {
    return enabled == TRUE;
  }
  return false;
}

bool is_window_cloaked(HWND window) {
  DWORD cloaked;
  HRESULT hr = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
  return SUCCEEDED(hr) && cloaked != 0;
}

bool get_window_image_by_dwm(HWND dwm_window, HWND window, const traa_size &target_size,
                             uint8_t **data, traa_size &scaled_size) {
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

// window

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-internalgetwindowtext
// [This function is not intended for general use. It may be altered or unavailable in subsequent
// versions of Windows.]
//
// Copies the text of the specified window's title bar (if it has one) into a buffer.
//
// This function is similar to the GetWindowText function. However, it obtains the window text
// directly from the window structure associated with the specified window's handle and then always
// provides the text as a Unicode string. This is unlike GetWindowText which obtains the text by
// sending the window a WM_GETTEXT message. If the specified window is a control, the text of the
// control is obtained.
//
// This function was not included in the SDK headers and libraries until Windows XP with Service
// Pack 1 (SP1) and Windows Server 2003.
int get_window_text_safe(HWND window, LPWSTR p_string, int cch_max_count) {
  // If the window has no title bar or text, if the title bar is empty, or if
  // the window or control handle is invalid, the return value is zero.
  return ::InternalGetWindowText(window, p_string, cch_max_count);
}

DWORD get_pid_by_window(HWND window) {
  DWORD pid = 0;
  ::GetWindowThreadProcessId(window, &pid);
  return pid;
}

bool is_window_chrome_notifaction(HWND window) {
  constexpr size_t k_title_length = 32;
  wchar_t window_title[k_title_length];
  get_window_text_safe(window, window_title, k_title_length);
  if (wcsnlen_s(window_title, k_title_length) != 0) {
    return false;
  }

  constexpr size_t k_class_length = 256;
  // Prefix used to match the window class for Chrome windows.
  constexpr wchar_t k_chrome_window_class_prefix[] = L"Chrome_WidgetWin_";

  wchar_t class_name[k_class_length];
  const int class_name_length = ::GetClassNameW(window, class_name, k_class_length);
  if (class_name_length < 1 ||
      wcsncmp(class_name, k_chrome_window_class_prefix,
              wcsnlen_s(k_chrome_window_class_prefix, k_class_length)) != 0) {
    return false;
  }

  const LONG exstyle = ::GetWindowLong(window, GWL_EXSTYLE);
  if ((exstyle & WS_EX_NOACTIVATE) && (exstyle & WS_EX_TOOLWINDOW) && (exstyle & WS_EX_TOPMOST)) {
    return true;
  }

  return false;
}

// `content_rect` is preferred because,
// 1. window_capturer_gdi is using GDI capturer, which cannot capture DX
// output.
//    So screen_capturer should be used as much as possible to avoid
//    uncapturable cases. Note: lots of new applications are using DX output
//    (hardware acceleration) to improve the performance which cannot be
//    captured by window_capturer_gdi. See bug http://crbug.com/741770.
// 2. window_capturer_gdi is still useful because we do not want to expose the
//    content on other windows if the target window is covered by them.
// 3. Shadow and borders should not be considered as "content" on other
//    windows because they do not expose any useful information.
//
// So we can bear the false-negative cases (target window is covered by the
// borders or shadow of other windows, but we have not detected it) in favor
// of using screen_capturer, rather than let the false-positive cases (target
// windows is only covered by borders or shadow of other windows, but we treat
// it as overlapping) impact the user experience.
bool is_window_overlapping(HWND window, HWND selected_hwnd,
                           const desktop_rect &selected_window_rect) {
  desktop_rect content_rect;
  if (!get_window_content_rect(window, &content_rect)) {
    // Bail out if failed to get the window area.
    return true;
  }
  content_rect.intersect_with(selected_window_rect);

  if (content_rect.is_empty()) {
    return false;
  }

  // When the taskbar is automatically hidden, it will leave a 2 pixel margin on
  // the screen which will overlap the maximized selected window that will use
  // up the full screen area. Since there is no solid way to identify a hidden
  // taskbar window, we have to make an exemption here if the overlapping is
  // 2 x screen_width/height to a maximized window.
  bool is_maximized = false;
  capture_utils::is_window_maximized(selected_hwnd, &is_maximized);

  // The hiddgen taskbar will leave a 2 pixel margin on the screen.
  constexpr int k_hidden_taskbar_margin_on_screen = 2;
  bool overlaps_hidden_horizontal_taskbar =
      selected_window_rect.width() == content_rect.width() &&
      content_rect.height() == k_hidden_taskbar_margin_on_screen;
  bool overlaps_hidden_vertical_taskbar = selected_window_rect.height() == content_rect.height() &&
                                          content_rect.width() == k_hidden_taskbar_margin_on_screen;
  if (is_maximized && (overlaps_hidden_horizontal_taskbar || overlaps_hidden_vertical_taskbar)) {
    return false;
  }

  return true;
}

bool has_active_display() {
  desktop_capturer::source_list_t screens;

  return get_screen_list(&screens) && !screens.empty();
}

bool get_screen_list(desktop_capturer::source_list_t *screens,
                     std::vector<std::string> *device_names /* = nullptr */) {
  BOOL enum_result = TRUE;
  for (int device_index = 0;; ++device_index) {
    DISPLAY_DEVICEW device;
    device.cb = sizeof(device);
    enum_result = ::EnumDisplayDevicesW(NULL, device_index, &device, 0);

    // `enum_result` is 0 if we have enumerated all devices.
    if (!enum_result) {
      break;
    }

    // We only care about active displays.
    if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
      continue;
    }

    screens->push_back({device_index, std::string()});
    if (device_names) {
      device_names->push_back(string_trans::unicode_to_utf8(device.DeviceName));
    }
  }
  return true;
}

bool get_hmonitor_from_device_index(const desktop_capturer::source_id_t device_index,
                                    HMONITOR *hmonitor) {
  // A device index of `k_screen_id_full` or -1 represents all screens, an
  // HMONITOR of 0 indicates the same.
  if (device_index == k_screen_id_full) {
    *hmonitor = 0;
    return true;
  }

  std::wstring device_key;
  if (!is_screen_valid(device_index, &device_key)) {
    return false;
  }

  desktop_rect screen_rect = get_screen_rect(device_index, device_key);
  if (screen_rect.is_empty()) {
    return false;
  }

  RECT rect = {screen_rect.left(), screen_rect.top(), screen_rect.right(), screen_rect.bottom()};

  HMONITOR monitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
  if (monitor == NULL) {
    LOG_WARN("No HMONITOR found for supplied device index.");
    return false;
  }

  *hmonitor = monitor;
  return true;
}

bool is_monitor_valid(const HMONITOR monitor) {
  // An HMONITOR of 0 refers to a virtual monitor that spans all physical
  // monitors.
  if (monitor == 0) {
    // There is a bug in a Windows OS API that causes a crash when capturing if
    // there are no active displays. We must ensure there is an active display
    // before returning true.
    if (!has_active_display())
      return false;

    return true;
  }

  MONITORINFO monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFO);
  return ::GetMonitorInfoA(monitor, &monitor_info);
}

desktop_rect get_monitor_rect(const HMONITOR monitor) {
  MONITORINFO monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFO);
  if (!::GetMonitorInfoA(monitor, &monitor_info)) {
    return desktop_rect();
  }

  return desktop_rect::make_ltrb(monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                                 monitor_info.rcMonitor.right, monitor_info.rcMonitor.bottom);
}

bool is_screen_valid(const desktop_capturer::source_id_t screen, std::wstring *device_key) {
  if (screen == k_screen_id_full) {
    *device_key = L"";
    return true;
  }

  DISPLAY_DEVICEW device;
  device.cb = sizeof(device);
  BOOL enum_result = ::EnumDisplayDevicesW(NULL, static_cast<DWORD>(screen), &device, 0);
  if (enum_result) {
    *device_key = device.DeviceKey;
  }

  return !!enum_result;
}

desktop_rect get_full_screen_rect() {
  return desktop_rect::make_xywh(
      ::GetSystemMetrics(SM_XVIRTUALSCREEN), ::GetSystemMetrics(SM_YVIRTUALSCREEN),
      ::GetSystemMetrics(SM_CXVIRTUALSCREEN), ::GetSystemMetrics(SM_CYVIRTUALSCREEN));
}

desktop_vector get_dpi_for_monitor(HMONITOR monitor) {
  UINT dpi_x, dpi_y;
  // MDT_EFFECTIVE_DPI includes the scale factor as well as the system DPI.
  HRESULT hr = ::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
  if (SUCCEEDED(hr)) {
    return {static_cast<INT>(dpi_x), static_cast<INT>(dpi_y)};
  }
  LOG_WARN("::GetDpiForMonitor() failed: {}", hr);

  // If we can't get the per-monitor DPI, then return the system DPI.
  HDC hdc = ::GetDC(nullptr);
  if (hdc) {
    desktop_vector dpi{::GetDeviceCaps(hdc, LOGPIXELSX), ::GetDeviceCaps(hdc, LOGPIXELSY)};
    ::ReleaseDC(nullptr, hdc);
    return dpi;
  }

  // If everything fails, then return the default DPI for Windows.
  return {96, 96};
}

desktop_rect get_screen_rect(const desktop_capturer::source_id_t screen,
                             const std::wstring &device_key) {
  if (screen == k_screen_id_full) {
    return get_full_screen_rect();
  }

  DISPLAY_DEVICEW device;
  device.cb = sizeof(device);
  BOOL result = ::EnumDisplayDevicesW(NULL, static_cast<DWORD>(screen), &device, 0);
  if (!result) {
    return desktop_rect();
  }

  // Verifies the device index still maps to the same display device, to make
  // sure we are capturing the same device when devices are added or removed.
  // DeviceKey is documented as reserved, but it actually contains the registry
  // key for the device and is unique for each monitor, while DeviceID is not.
  if (device_key != device.DeviceKey) {
    return desktop_rect();
  }

  DEVMODEW device_mode;
  device_mode.dmSize = sizeof(device_mode);
  device_mode.dmDriverExtra = 0;
  result = ::EnumDisplaySettingsExW(device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0);
  if (!result) {
    return desktop_rect();
  }

  return desktop_rect::make_xywh(device_mode.dmPosition.x, device_mode.dmPosition.y,
                                 device_mode.dmPelsWidth, device_mode.dmPelsHeight);
}

} // namespace capture_utils

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

// wrl
window_capture_helper_win::window_capture_helper_win() {
  if (os_get_version() >= version_alias::VERSION_WIN10) {
    if (FAILED(::CoCreateInstance(__uuidof(VirtualDesktopManager), nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&virtual_desktop_manager_)))) {
      LOG_WARN("fail to create instance of VirtualDesktopManager");
    }
  }
}

window_capture_helper_win::~window_capture_helper_win() {}

bool window_capture_helper_win::is_window_on_current_desktop(HWND window) {
  // Make sure the window is on the current virtual desktop.
  if (virtual_desktop_manager_) {
    BOOL on_current_desktop;
    if (SUCCEEDED(virtual_desktop_manager_->IsWindowOnCurrentVirtualDesktop(window,
                                                                            &on_current_desktop)) &&
        !on_current_desktop) {
      return false;
    }
  }
  return true;
}

bool window_capture_helper_win::is_window_visible_on_current_desktop(HWND window) {
  return capture_utils::is_window_valid_and_visible(window) &&
         is_window_on_current_desktop(window) && !capture_utils::is_window_cloaked(window);
}

bool window_capture_helper_win::enumerate_capturable_windows(
    desktop_capturer::source_list_t *results, bool enumerate_current_process_windows,
    LONG ex_style_filters) {
  int flags =
      (TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN | TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNRESPONSIVE);

  if (!enumerate_current_process_windows) {
    flags |= TRAA_SCREEN_SOURCE_FLAG_IGNORE_CURRENT_PROCESS_WINDOWS;
  }

  if (!(ex_style_filters & WS_EX_TOOLWINDOW)) {
    flags |= TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_TOOLWINDOW;
  }

  // Note: enum_screen_source_info will do more than the original function of webrtc, such as get
  // the process path of the window. which may cost more time, so we should consider to provide a
  // faster way to get the window list, if this become a performance issue.
  traa_size empty_size;
  traa_screen_source_info *infos = nullptr;
  int count = 0;
  if (screen_source_info_enumerator::enum_screen_source_info(empty_size, empty_size, flags, &infos,
                                                             &count) != TRAA_ERROR_NONE) {
    return false;
  }

  for (int i = 0; i < count; i++) {
    if (!is_window_visible_on_current_desktop(reinterpret_cast<HWND>(infos[i].id))) {
      continue;
    }

    desktop_capturer::source_t source;
    source.id = static_cast<desktop_capturer::source_id_t>(infos[i].id);
    // Note: this may copy twice, we should consider to optimize this if this become a performance
    // issue.
    source.title = infos[i].title;
    source.display_id = infos[i].screen_id;

    results->push_back(source);
  }

  if (infos) {
    screen_source_info_enumerator::free_screen_source_info(infos, count);
  }

  return true;
}

} // namespace base
} // namespace traa