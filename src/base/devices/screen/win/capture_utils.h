#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_

#include <traa/base.h>

#include "base/devices/screen/desktop_capturer.h"
#include "base/disallow.h"

#include <Windows.h>

// for Microsoft::WRL::ComPtr<IVirtualDesktopManager>
#include <ShlObj.h>
#include <wrl/client.h>

#include <string>
#include <vector>

namespace traa {
namespace base {

class desktop_rect;

// capture utils
namespace capture_utils {
// dpi
float get_dpi_scale_for_process(HANDLE process);
float get_dpi_scale_for_process();

float get_dpi_scale(HWND window);

bool is_dpi_aware();
bool is_dpi_aware(HWND window);

// gdi
void dump_bmp(const uint8_t *data, const traa_size &size, const char *file_name);

// Checks if a window responds to a message within 50ms.
bool is_window_response(HWND window);

// Retrieves whether the `window` is maximized and stores in `result`. This
// function returns false if native APIs fail.
bool is_window_maximized(HWND window, bool *result);

// Checks that the HWND is for a valid window, that window's visibility state is
// visible, and that it is not minimized.
bool is_window_valid_and_visible(HWND window);

bool is_window_owned_by_current_process(HWND window);

// Outputs the window rect. The returned desktop_rect is in system coordinates,
// i.e. the primary monitor on the system always starts from (0, 0). This
// function returns false if native APIs fail.
bool get_window_rect(HWND window, desktop_rect *rect);

// Outputs the window rect, with the left/right/bottom frame border cropped if
// the window is maximized or has a transparent resize border.
// `avoid_cropping_border` may be set to true to avoid cropping the visible
// border when cropping any resize border.
// `cropped_rect` is the cropped rect relative to the
// desktop. `original_rect` is the original rect returned from GetWindowRect.
// Returns true if all API calls succeeded. The returned desktop_rect is in
// system coordinates, i.e. the primary monitor on the system always starts from
// (0, 0). `original_rect` can be nullptr.
//
// TODO(zijiehe): Move this function to CroppingWindowCapturerWin after it has
// been removed from MouseCursorMonitorWin.
// This function should only be used by CroppingWindowCapturerWin. Instead a
// desktop_rect CropWindowRect(const desktop_rect& rect)
// should be added as a utility function to help CroppingWindowCapturerWin and
// WindowCapturerWinGdi to crop out the borders or shadow according to their
// scenarios. But this function is too generic and easy to be misused.
bool get_window_cropped_rect(HWND window, bool avoid_cropping_border, desktop_rect *cropped_rect,
                             desktop_rect *original_rect);

// Retrieves the rectangle of the content area of `window`. Usually it contains
// title bar and window client area, but borders or shadow are excluded. The
// returned desktop_rect is in system coordinates, i.e. the primary monitor on
// the system always starts from (0, 0). This function returns false if native
// APIs fail.
bool get_window_content_rect(HWND window, desktop_rect *result);

// Returns the region type of the `window` and fill `rect` with the region of
// `window` if region type is SIMPLEREGION.
int get_window_region_type_with_boundary(HWND window, desktop_rect *result);

// Retrieves the size of the `hdc`. This function returns false if native APIs
// fail.
bool get_dc_size(HDC hdc, desktop_size *size);

bool get_window_image_by_gdi(HWND window, const traa_size &target_size, uint8_t **data,
                             traa_size &scaled_size);

bool get_screen_image_by_gdi(const traa_rect &rect, const traa_size &target_size, uint8_t **data,
                             traa_size &scaled_size);

// dwm
bool is_dwm_composition_enabled();

// A cloaked window is composited but not visible to the user.
// Example: Cortana or the Action Center when collapsed.
bool is_window_cloaked(HWND window);

bool get_window_image_by_dwm(HWND dwm_window, HWND window, const traa_size &target_size,
                             uint8_t **data, traa_size &scaled_size);

// window
int get_window_text_safe(HWND window, LPWSTR p_string, int cch_max_count);

DWORD get_pid_by_window(HWND window);

// This is just a best guess of a notification window. Chrome uses the Windows
// native framework for showing notifications. So far what we know about such a
// window includes: no title, class name with prefix "Chrome_WidgetWin_" and
// with certain extended styles.
bool is_window_chrome_notifaction(HWND window);

bool is_window_overlapping(HWND window, HWND selected_hwnd,
                           const desktop_rect &selected_window_rect);

// display
// Returns true if the system has at least one active display.
bool has_active_display();

// Output the list of active screens into `screens`. Returns true if succeeded,
// or false if it fails to enumerate the display devices. If the `device_names`
// is provided, it will be filled with the DISPLAY_DEVICE.DeviceName in UTF-8
// encoding. If this function returns true, consumers can always assume that
// `screens`[i] and `device_names`[i] indicate the same monitor on the system.
bool get_screen_list(desktop_capturer::source_list_t *screens,
                     std::vector<std::string> *device_names = nullptr);

// Converts a device index (which are returned by `GetScreenList`) into an
// HMONITOR.
bool get_hmonitor_from_device_index(desktop_capturer::source_id_t device_index, HMONITOR *hmonitor);

// Returns true if `monitor` represents a valid display
// monitor. Consumers should recheck the validity of HMONITORs before use if a
// WM_DISPLAYCHANGE message has been received.
bool is_monitor_valid(HMONITOR monitor);

// Returns the rect of the monitor identified by `monitor`, relative to the
// primary display's top-left. On failure, returns an empty rect.
desktop_rect get_monitor_rect(HMONITOR monitor);

// Returns the DPI for the specified monitor. On failure, returns the system DPI
// or the Windows default DPI (96x96) if the system DPI can't be retrieved.
desktop_vector get_dpi_for_monitor(HMONITOR monitor);

// Returns true if `screen` is a valid screen. The screen device key is
// returned through `device_key` if the screen is valid. The device key can be
// used in GetScreenRect to verify the screen matches the previously obtained
// id.
bool is_screen_valid(desktop_capturer::source_id_t screen, std::wstring *device_key);

// Get the rect of the entire system in system coordinate system. I.e. the
// primary monitor always starts from (0, 0).
desktop_rect get_full_screen_rect();

// Get the rect of the screen identified by `screen`, relative to the primary
// display's top-left. If the screen device key does not match `device_key`, or
// the screen does not exist, or any error happens, an empty rect is returned.
desktop_rect get_screen_rect(desktop_capturer::source_id_t screen, const std::wstring &device_key);

} // namespace capture_utils

// thumbnail
class thumbnail {
public:
  thumbnail();
  ~thumbnail();

  bool get_thumbnail_data(HWND window, const traa_size &thumbnail_size, uint8_t **data,
                          traa_size &size);

private:
  HWND dwm_window_;

private:
  DISALLOW_COPY_AND_ASSIGN(thumbnail);
};

// wrl
class window_capture_helper_win {
public:
  window_capture_helper_win();
  ~window_capture_helper_win();

  window_capture_helper_win(const window_capture_helper_win &) = delete;
  window_capture_helper_win &operator=(const window_capture_helper_win &) = delete;

  bool is_window_on_current_desktop(HWND window);
  bool is_window_visible_on_current_desktop(HWND window);

  // The optional `ex_style_filters` parameter allows callers to provide
  // extended window styles (e.g. WS_EX_TOOLWINDOW) and prevent windows that
  // match from being included in `results`.
  bool enumerate_capturable_windows(desktop_capturer::source_list_t *results,
                                    bool enumerate_current_process_windows,
                                    LONG ex_style_filters = 0);

private:
  // Only used on Win10+.
  Microsoft::WRL::ComPtr<IVirtualDesktopManager> virtual_desktop_manager_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_CAPTURE_UTILS_H_