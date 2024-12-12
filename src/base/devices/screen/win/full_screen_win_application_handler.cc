/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen//win/full_screen_win_application_handler.h"

#include "base/arraysize.h"
#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/win/capture_utils.h"
#include "base/folder/folder.h"
#include "base/strings/ascii.h"
#include "base/strings/string_trans.h"

#include "base/log/logger.h"

#include <algorithm>
#include <cwctype>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace traa {
namespace base {

inline namespace {

// Utility function to verify that `window` has class name equal to `class_name`
bool has_class_name(HWND window, const wchar_t *class_name) {
  const size_t name_len = wcslen(class_name);

  // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassa
  // says lpszClassName field in WNDCLASS is limited by 256 symbols, so we don't
  // need to have a buffer bigger than that.
  constexpr size_t k_max_name_len = 256;
  wchar_t buffer[k_max_name_len];

  const int length = ::GetClassNameW(window, buffer, k_max_name_len);
  if (length <= 0)
    return false;

  if (static_cast<size_t>(length) != name_len)
    return false;
  return wcsncmp(buffer, class_name, name_len) == 0;
}

std::string get_window_title(HWND window) {
  constexpr int k_max_title_length = 256;
  wchar_t title[k_max_title_length] = {0};
  int len = capture_utils::get_window_text_safe(window, title, k_max_title_length - 1);
  if (len == 0)
    return std::string();

  return string_trans::unicode_to_utf8(title);
}

// Returns windows which belong to given process id
// `sources` is a full list of available windows
// `pid` is a process identifier (window owner)
// `window_to_exclude` is a window to be exluded from result
desktop_capturer::source_list_t find_windows_by_pid(const desktop_capturer::source_list_t &sources,
                                                    DWORD pid, HWND window_to_exclude) {
  desktop_capturer::source_list_t result;
  std::copy_if(sources.begin(), sources.end(), std::back_inserter(result),
               [&](desktop_capturer::source_t source) {
                 const HWND source_hwnd = reinterpret_cast<HWND>(source.id);
                 return window_to_exclude != source_hwnd &&
                        capture_utils::get_pid_by_window(source_hwnd) == pid;
               });
  return result;
}

class power_point_handler : public full_screen_app_handler {
public:
  explicit power_point_handler(desktop_capturer::source_id_t source_id_t)
      : full_screen_app_handler(source_id_t) {}

  ~power_point_handler() override {}

  desktop_capturer::source_id_t
  find_full_screen_window(const desktop_capturer::source_list_t &window_list,
                          int64_t timestamp) const override {
    if (window_list.empty())
      return 0;

    HWND original_window = reinterpret_cast<HWND>(get_source_id());
    DWORD process_id = capture_utils::get_pid_by_window(original_window);

    desktop_capturer::source_list_t powerpoint_windows =
        find_windows_by_pid(window_list, process_id, original_window);

    if (powerpoint_windows.empty())
      return 0;

    if (get_window_type(original_window) != window_type::editor)
      return 0;

    const auto original_document = get_document_from_editor(original_window);

    for (const auto &source : powerpoint_windows) {
      HWND window = reinterpret_cast<HWND>(source.id);

      // Looking for slide show window for the same document
      if (get_window_type(window) != window_type::slide_show ||
          get_document_from_slide(window) != original_document) {
        continue;
      }

      return source.id;
    }

    return 0;
  }

private:
  enum class window_type { editor, slide_show, other };

  window_type get_window_type(HWND window) const {
    if (is_editor_window(window))
      return window_type::editor;
    else if (is_slide_show_window(window))
      return window_type::slide_show;
    else
      return window_type::other;
  }

  constexpr static char k_document_title_separator[] = " - ";

  std::string get_document_from_editor(HWND window) const {
    std::string title = get_window_title(window);
    auto position = title.find(k_document_title_separator);
    return std::string(strip_ascii_whitespace(std::string_view(title).substr(0, position)));
  }

  std::string get_document_from_slide(HWND window) const {
    std::string title = get_window_title(window);
    auto left_pos = title.find(k_document_title_separator);
    auto right_pos = title.rfind(k_document_title_separator);
    constexpr size_t k_separator_length = arraysize(k_document_title_separator) - 1;
    if (left_pos == std::string::npos || right_pos == std::string::npos)
      return title;

    if (right_pos > left_pos + k_separator_length) {
      auto result_len = right_pos - left_pos - k_separator_length;
      auto document = std::string_view(title).substr(left_pos + k_separator_length, result_len);
      return std::string(strip_ascii_whitespace(document));
    } else {
      auto document =
          std::string_view(title).substr(left_pos + k_separator_length, std::wstring::npos);
      return std::string(strip_ascii_whitespace(document));
    }
  }

  bool is_editor_window(HWND window) const { return has_class_name(window, L"PPTFrameClass"); }

  bool is_slide_show_window(HWND window) const {
    const LONG style = ::GetWindowLong(window, GWL_STYLE);
    const bool min_box = WS_MINIMIZEBOX & style;
    const bool max_box = WS_MAXIMIZEBOX & style;
    return !min_box && !max_box;
  }
};

class open_office_application_handler : public full_screen_app_handler {
public:
  explicit open_office_application_handler(desktop_capturer::source_id_t source_id_t)
      : full_screen_app_handler(source_id_t) {}

  desktop_capturer::source_id_t
  find_full_screen_window(const desktop_capturer::source_list_t &window_list,
                          int64_t timestamp) const override {
    if (window_list.empty())
      return 0;

    DWORD process_id = capture_utils::get_pid_by_window(reinterpret_cast<HWND>(get_source_id()));

    desktop_capturer::source_list_t app_windows =
        find_windows_by_pid(window_list, process_id, nullptr);

    desktop_capturer::source_list_t document_windows;
    std::copy_if(app_windows.begin(), app_windows.end(), std::back_inserter(document_windows),
                 [this](const desktop_capturer::source_t &x) { return is_editor_window(x); });

    // Check if we have only one document window, otherwise it's not possible
    // to securely match a document window and a slide show window which has
    // empty title.
    if (document_windows.size() != 1) {
      return 0;
    }

    // Check if document window has been selected as a source
    if (document_windows.front().id != get_source_id()) {
      return 0;
    }

    // Check if we have a slide show window.
    auto slide_show_window = std::find_if(
        app_windows.begin(), app_windows.end(),
        [this](const desktop_capturer::source_t &x) { return is_slide_show_window(x); });

    if (slide_show_window == app_windows.end())
      return 0;

    return slide_show_window->id;
  }

private:
  bool is_editor_window(const desktop_capturer::source_t &source) const {
    if (source.title.empty()) {
      return false;
    }

    return has_class_name(reinterpret_cast<HWND>(source.id), L"SALFRAME");
  }

  bool is_slide_show_window(const desktop_capturer::source_t &source) const {
    // Check title size to filter out a Presenter Control window which shares
    // window class with Slide Show window but has non empty title.
    if (!source.title.empty()) {
      return false;
    }

    return has_class_name(reinterpret_cast<HWND>(source.id), L"SALTMPSUBFRAME");
  }
};

std::wstring get_exe_path_by_window_id(HWND window_id) {
  DWORD process_id = capture_utils::get_pid_by_window(window_id);
  HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
  if (process == NULL)
    return L"";
  DWORD path_len = MAX_PATH;
  wchar_t path[MAX_PATH];
  std::wstring result;
  if (::QueryFullProcessImageNameW(process, 0, path, &path_len))
    result = std::wstring(path, path_len);
  else
    LOG_ERROR("QueryFullProcessImageName failed, {}", GetLastError());

  ::CloseHandle(process);
  return result;
}

std::wstring find_name_from_path(const std::wstring &path) {
  auto found = path.rfind(L"\\");
  if (found == std::string::npos)
    return path;
  return path.substr(found + 1);
}

} // namespace

std::unique_ptr<full_screen_app_handler>
create_full_screen_app_handler(desktop_capturer::source_id_t source_id_t) {
  std::unique_ptr<full_screen_app_handler> result;
  HWND hwnd = reinterpret_cast<HWND>(source_id_t);
  std::wstring exe_path = get_exe_path_by_window_id(hwnd);

  std::wstring file_name = find_name_from_path(exe_path);
  std::transform(file_name.begin(), file_name.end(), file_name.begin(), std::towupper);

  auto is_ends_with = [](std::string_view text, std::string_view suffix) {
    return suffix.empty() ||
           (text.size() >= suffix.size() &&
            memcmp(text.data() + (text.size() - suffix.size()), suffix.data(), suffix.size()) == 0);
  };

  if (file_name == L"POWERPNT.EXE") {
    result = std::make_unique<power_point_handler>(source_id_t);
  } else if (file_name == L"SOFFICE.BIN" &&
             is_ends_with(get_window_title(hwnd), "OpenOffice Impress")) {
    result = std::make_unique<open_office_application_handler>(source_id_t);
  }

  return result;
}

} // namespace base
} // namespace traa
