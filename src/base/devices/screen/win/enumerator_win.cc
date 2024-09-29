#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/enumerator.h"
#include "base/devices/screen/mouse_cursor.h"
#include "base/devices/screen/win/cursor.h"
#include "base/devices/screen/win/scoped_object_gdi.h"
#include "base/log/logger.h"
#include "base/strings/string_trans.h"
#include "base/utils/win/version.h"

#include <libyuv/scale_argb.h>

#include <memory>
#include <string>

#include <stdlib.h>

#include <shellapi.h>
#include <windows.h>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

struct enumerator_param {
  traa_size icon_size;
  traa_size thumbnail_size;
  unsigned int external_flags;
  std::vector<traa_screen_source_info> infos;
};

// https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute
// Minimum supported client	Windows Vista [desktop apps only]
// Retrieves the current value of a specified Desktop Window Manager (DWM) attribute applied to a
// window. For programming guidance, and code examples, see Controlling non-client region rendering.
typedef HRESULT(WINAPI *FuncDwmGetWindowAttribute)(HWND window, DWORD dwAttribute,
                                                   PVOID pvAttribute, DWORD cbAttribute);

FuncDwmGetWindowAttribute helper_get_dwmapi_get_window_attribute() {
  HINSTANCE dwmapi = LoadLibraryW(L"Dwmapi.dll");
  if (dwmapi == nullptr) {
    return nullptr;
  }

  FuncDwmGetWindowAttribute dwmapi_get_window_attribute =
      (FuncDwmGetWindowAttribute)GetProcAddress(dwmapi, "DwmGetWindowAttribute");
  if (dwmapi_get_window_attribute == nullptr) {
    return nullptr;
  }

  return dwmapi_get_window_attribute;
}

bool is_window_invisible_win10_background_app(HWND window) {
  HINSTANCE dwmapi = LoadLibraryW(L"Dwmapi.dll");
  if (dwmapi != nullptr) {
    auto dwmapi_get_window_attribute = helper_get_dwmapi_get_window_attribute();
    if (!dwmapi_get_window_attribute) {
      return false;
    }

    int cloaked_val = 0;
    HRESULT hres = dwmapi_get_window_attribute(window, 14 /*DWMWA_CLOAKED*/, &cloaked_val,
                                               sizeof(cloaked_val));
    if (hres != S_OK) {
      cloaked_val = 0;
    }
    return cloaked_val ? true : false;
  }
  return false;
}

bool is_window_responding(HWND window) {
  // 50ms is chosen in case the system is under heavy load, but it's also not
  // too long to delay window enumeration considerably.
  const UINT timeout_ms = 50;
  return ::SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, timeout_ms, nullptr);
}

bool is_window_owned_by_current_process(HWND window) {
  DWORD process_id;
  ::GetWindowThreadProcessId(window, &process_id);
  return process_id == ::GetCurrentProcessId();
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

int get_window_process_path(HWND window, wchar_t *path, int max_count) {
  DWORD process_id;
  ::GetWindowThreadProcessId(window, &process_id);
  if (process_id == 0) {
    return 0;
  }

  HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
  if (process == nullptr) {
    return 0;
  }

  DWORD buffer_size = static_cast<DWORD>(max_count);
  if (::QueryFullProcessImageNameW(process, 0, path, &buffer_size) == 0) {
    ::CloseHandle(process);
    return 0;
  }

  ::CloseHandle(process);
  return buffer_size;
}

int64_t get_window_owned_screen_id(const HWND window) {
  if (!window)
    return TRAA_INVALID_SCREEN_ID;

  // always return nullptr when the window intersects no display monitor
  // rectangles.
  HMONITOR hmonitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
  if (hmonitor == nullptr)
    return TRAA_INVALID_SCREEN_ID;

  MONITORINFOEXW monitor_info_ex;
  monitor_info_ex.cbSize = sizeof(MONITORINFOEXW);
  if (!::GetMonitorInfoW(hmonitor, &monitor_info_ex))
    return TRAA_INVALID_SCREEN_ID;

  std::wstring device_name = monitor_info_ex.szDevice;

  int64_t screen_id = TRAA_INVALID_SCREEN_ID;
  BOOL enum_result = TRUE;
  for (int device_index = 0;; ++device_index) {
    DISPLAY_DEVICEW device;
    device.cb = sizeof(device);
    enum_result = EnumDisplayDevicesW(NULL, device_index, &device, 0);

    // |enum_result| is 0 if we have enumerated all devices.
    if (!enum_result) {
      break;
    }

    // We only care about active displays.
    if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE)) {
      continue;
    }

    if (device_name == device.DeviceName) {
      screen_id = device_index;
      break;
    }
  }
  return screen_id;
}

bool get_window_owned_screen_work_rect(HWND window, desktop_rect *rect) {
  if (!window)
    return false;

  // always return empty rect when the window intersects no display monitor
  // rectangles.
  HMONITOR hmonitor = NULL;
  if ((hmonitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONULL)) == NULL)
    return false;

  MONITORINFOEXW info_ex;
  info_ex.cbSize = sizeof(MONITORINFOEXW);
  if (!::GetMonitorInfoW(hmonitor, &info_ex))
    return false;

  *rect = desktop_rect::make_ltrb(info_ex.rcWork.left, info_ex.rcWork.top, info_ex.rcWork.right,
                                  info_ex.rcWork.bottom);

  return true;
}

bool get_window_rect(HWND window, desktop_rect *rect) {
  RECT rc;
  if (!::GetWindowRect(window, &rc)) {
    return false;
  }

  *rect = desktop_rect::make_ltrb(rc.left, rc.top, rc.right, rc.bottom);

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
    auto dwmapi_get_window_attribute = helper_get_dwmapi_get_window_attribute();
    if (!dwmapi_get_window_attribute) {
      return false;
    }

    // Only apply this cropping to windows with a resize border (otherwise,
    // it'd clip the edges of captured pop-up windows without this border).
    RECT rect;
    dwmapi_get_window_attribute(window, 9 /*DWMWINDOWATTRIBUTE::DWMWA_EXTENDED_FRAME_BOUNDS = 9*/,
                                &rect, sizeof(RECT));
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
  // GetWindowRect().
  if (width > 0 && width < result->width()) {
    // - GetClientRect() always set the left / top of RECT to 0. So we need to
    //   estimate the border width from GetClientRect() and GetWindowRect().
    // - Border width of a window varies according to the window type.
    // - GetClientRect() excludes the title bar, which should be considered as
    //   part of the content and included in the captured frame. So we always
    //   estimate the border width according to the window width.
    // - We assume a window has same border width in each side.
    // So we shrink half of the width difference from all four sides.
    const int shrink = ((width - result->width()) / 2);
    // When `shrink` is negative, DesktopRect::Extend() shrinks itself.
    result->extend(shrink, 0, shrink, 0);
    // Usually this should not happen, just in case we have received a strange
    // window, which has only left and right borders.
    if (result->height() > shrink * 2) {
      result->extend(0, shrink, 0, shrink);
    }
  }

  return true;
}

bool get_window_maximized_rect(HWND window, desktop_rect *intersects_rect) {
  if (!get_window_rect(window, intersects_rect))
    return false;

  desktop_rect work_rect;
  if (!get_window_owned_screen_work_rect(window, &work_rect))
    return false;

  intersects_rect->intersect_width(work_rect);
  return true;
}

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
  double scale_factor = sqrt(static_cast<double>(dst_image_size) / src_image_size);

  // The codec requires the width and height to be multiples of 2.
  int32_t scaled_width = static_cast<int32_t>(source.width() * scale_factor) & 0xFFFFFFFE;
  int32_t scaled_height = static_cast<int32_t>(source.height() * scale_factor) & 0xFFFFFFFE;

  return desktop_size(scaled_width, scaled_height);
}

bool get_process_icon_data(LPCWSTR process_path, desktop_size icon_size, uint8_t **icon_data) {
  HICON hicon = nullptr;
  ::ExtractIconExW(process_path, 0, &hicon, nullptr, 1);

  // if the icon is not found, use the default icon from shell32.dll
  if (!hicon) {
    LOG_WARN("extract icon from {} failed, use the default icon from shell32.dll",
             string_trans::unicode_to_utf8(process_path));
    ::ExtractIconExW(L"shell32.dll", 2, &hicon, nullptr, 1);
  }

  if (!hicon) {
    LOG_ERROR("extract icon from {} failed: {}", string_trans::unicode_to_utf8(process_path),
              ::GetLastError());
    return false;
  }

  scoped_icon icon(hicon);

  std::unique_ptr<mouse_cursor> cursor(create_mouse_cursor_from_handle(::GetDC(NULL), icon));
  if (!cursor) {
    LOG_ERROR("create mouse cursor from handle failed: {}", ::GetLastError());
    return false;
  }

  auto scaled_size = calc_scaled_size(cursor->image()->size(), icon_size);
  if (scaled_size.is_empty()) {
    LOG_ERROR("calc scaled size failed, get empty size");
    return false;
  }

  auto data_size = scaled_size.width() * scaled_size.height() * 4;

  *icon_data = new uint8_t[data_size];
  if (!*icon_data) {
    LOG_ERROR("alloca memroy for icon data with size {} failed: {}", data_size, ::GetLastError());
    return false;
  }

  libyuv::ARGBScale(cursor->image()->data(), cursor->image()->stride(),
                    cursor->image()->size().width(), cursor->image()->size().height(), *icon_data,
                    scaled_size.width() * desktop_frame::kBytesPerPixel, scaled_size.width(),
                    scaled_size.height(), libyuv::kFilterBox);

  return true;
}

BOOL WINAPI enum_screen_source_info_proc(HWND window, LPARAM lParam) {
  auto *param = reinterpret_cast<enumerator_param *>(lParam);

  // skip invisible and minimized windows and the shell window
  if (!::IsWindowVisible(window) || ::IsIconic(window) || ::GetShellWindow() == window) {
    return TRUE;
  }

  // skip windows if it is not the root window
  if (::GetAncestor(window, GA_ROOT) != window) {
    return TRUE;
  }

  // skip windows if the window rect is empty
  desktop_rect window_rect = desktop_rect::make_ltrb(0, 0, 0, 0);
  if (!get_window_rect(window, &window_rect) || window_rect.is_empty()) {
    return TRUE;
  }

  // skip windows which are background app windows on Windows 10
  if (is_window_invisible_win10_background_app(window)) {
    return TRUE;
  }

  // skip windows which are not presented in the taskbar,
  // namely owned window if they don't have the app window style set
  HWND owner = ::GetWindow(window, GW_OWNER);
  LONG exstyle = ::GetWindowLongW(window, GWL_EXSTYLE);
  if (owner && !(exstyle & WS_EX_APPWINDOW)) {
    return TRUE;
  }

  // skip WS_EX_TOOLWINDOW unless the TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_TOOLWINDOW flag is set.
  if ((exstyle & WS_EX_TOOLWINDOW) &&
      !(param->external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_TOOLWINDOW)) {
    return TRUE;
  }

  // skip windows which are not responding unless the TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNRESPONSIVE flag
  // is set.
  if (!is_window_responding(window) &&
      !(param->external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNRESPONSIVE)) {
    return TRUE;
  }

  // GetWindowText* are potentially blocking operations if `hwnd` is
  // owned by the current process. The APIs will send messages to the window's
  // message loop, and if the message loop is waiting on this operation we will
  // enter a deadlock.
  // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowtexta#remarks
  //
  // To help consumers avoid this, there is a DesktopCaptureOption to ignore
  // windows owned by the current process. Consumers should either ensure that
  // the thread running their message loop never waits on this operation, or use
  // the option to exclude these windows from the source list.
  //
  // skip windows owned by the current process if the TRAA_SCREEN_SOURCE_FLAG_IGNORE_CURRENT_PROCESS
  // flag is set.
  bool owned_by_current_process = is_window_owned_by_current_process(window);
  if ((param->external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_CURRENT_PROCESS) &&
      owned_by_current_process) {
    return TRUE;
  }

  bool has_title = false;
  WCHAR window_title[TRAA_MAX_DEVICE_NAME_LENGTH] = L"";
  if (get_window_text_safe(window, window_title, TRAA_MAX_DEVICE_NAME_LENGTH - 1) > 0) {
    has_title = true;
  } else {
    LOG_ERROR("get window title failed: {}", ::GetLastError());
  }

  // skip windows when we failed to convert the title or it is empty unless the
  // TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNTITLED flag is set.
  if (!has_title && !(param->external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNTITLED)) {
    return TRUE;
  }

  bool has_process_path = false;
  WCHAR process_path[TRAA_MAX_DEVICE_NAME_LENGTH] = L"";
  if (get_window_process_path(window, process_path, TRAA_MAX_DEVICE_NAME_LENGTH - 1) > 0) {
    has_process_path = true;
  } else {
    LOG_ERROR("get window process path failed: {}", ::GetLastError());
  }

  if ((param->external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_NOPROCESS_PATH) &&
      !has_process_path) {
    return TRUE;
  }

  // capture the window class name, to allow specific window classes to be
  // skipped.
  //
  // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // says lpszClassName field in WNDCLASS is limited by 256 symbols, so we don't
  // need to have a buffer bigger than that.
  WCHAR class_name[TRAA_MAX_DEVICE_NAME_LENGTH] = L"";
  const int class_name_length = ::GetClassNameW(window, class_name, TRAA_MAX_DEVICE_NAME_LENGTH);
  if (class_name_length < 1)
    return TRUE;

  if (!(param->external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_SKIP_SYSTEM_WINDOWS)) {
    // skip program manager window.
    if (wcscmp(class_name, L"Progman") == 0 || wcscmp(class_name, L"Program Manager") == 0)
      return TRUE;

    // skip task manager window.
    if (wcscmp(class_name, L"TaskManagerWindow") == 0)
      return TRUE;

    // skip Start button window on Windows Vista, Windows 7.
    // on Windows 8, Windows 8.1, Windows 10 Start button is not a top level
    // window, so it will not be examined here.
    if (wcscmp(class_name, L"Button") == 0)
      return TRUE;

    // skip tab proxy windows of Edge
    if (wcscmp(class_name, L"Windows.Internal.Shell.TabProxyWindow") == 0)
      return TRUE;
  }

  // get window info, this should be placed after all the skip conditions
  traa_screen_source_info window_info;
  window_info.id = reinterpret_cast<int64_t>(window);
  window_info.screen_id = get_window_owned_screen_id(window);
  window_info.is_window = true;
  window_info.is_minimized = ::IsIconic(window);

  if (is_window_maximized(window, &window_info.is_maximized) && window_info.is_maximized) {
    get_window_maximized_rect(window, &window_rect);
  }
  window_info.rect = window_rect.to_traa_rect();
  window_info.icon_size = param->icon_size;
  window_info.thumbnail_size = param->thumbnail_size;

  // title
  if (has_title) {
    auto utf8_title = string_trans::unicode_to_utf8(window_title);
    strncpy_s(const_cast<char *>(window_info.title), sizeof(window_info.title) - 1,
              utf8_title.c_str(), utf8_title.length());
  }

  // process path
  if (has_process_path) {
    auto utf8_process_path = string_trans::unicode_to_utf8(process_path);
    strncpy_s(const_cast<char *>(window_info.process_path), sizeof(window_info.process_path) - 1,
              utf8_process_path.c_str(), utf8_process_path.length());
  }

  // get the icon data
  if (has_process_path && param->icon_size.width > 0 && param->icon_size.height > 0 &&
      get_process_icon_data(process_path,
                            desktop_size(param->icon_size.width, param->icon_size.height),
                            const_cast<uint8_t **>(&window_info.icon_data))) {
    window_info.icon_size = param->icon_size;
  }

  // get the thumbnail data

  // push the window to the list
  param->infos.push_back(window_info);

  return TRUE;
}

int screen_source_info_enumerator::enum_screen_source_info(const traa_size icon_size,
                                                           const traa_size thumbnail_size,
                                                           const unsigned int external_flags,
                                                           traa_screen_source_info **infos,
                                                           int *count) {
  enumerator_param param = {icon_size, thumbnail_size, external_flags, {}};

  BOOL ret = ::EnumWindows(enum_screen_source_info_proc, reinterpret_cast<LPARAM>(&param));
  if (!ret) {
    LOG_ERROR("call ::EnumWindows failed: {}", ::GetLastError());
    return traa_error::TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED;
  }

  *count = static_cast<int>(param.infos.size());
  *infos =
      reinterpret_cast<traa_screen_source_info *>(new traa_screen_source_info[param.infos.size()]);
  if (*infos == nullptr) {
    LOG_ERROR("alloca memroy for infos failed: {}", ::GetLastError());
    return traa_error::TRAA_ERROR_OUT_OF_MEMORY;
  }

  for (size_t i = 0; i < param.infos.size(); ++i) {
    auto &source_info = param.infos[i];
    auto &dest_info = (*infos)[i];
    memcpy(&dest_info, &source_info, sizeof(traa_screen_source_info));
    strncpy_s(const_cast<char *>(dest_info.title), sizeof(dest_info.title) - 1, source_info.title,
              std::strlen(source_info.title));
    strncpy_s(const_cast<char *>(dest_info.process_path), sizeof(dest_info.process_path) - 1,
              source_info.process_path, std::strlen(source_info.process_path));
  }

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa