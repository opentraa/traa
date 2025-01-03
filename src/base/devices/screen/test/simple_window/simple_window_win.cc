#include "base/devices/screen/test/simple_window/simple_window.h"

#include <windows.h>

namespace traa {
namespace base {

class simple_window_win : public simple_window {
  const std::wstring k_window_class_name = L"simple_window_win";

  static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_PAINT:
      PAINTSTRUCT paint_struct;
      HDC hdc = ::BeginPaint(hwnd, &paint_struct);

      // Paint the window so the color is consistent and we can inspect the
      // pixels in tests and know what to expect.
      ::FillRect(hdc, &paint_struct.rcPaint,
                 ::CreateSolidBrush(RGB(k_r_value, k_g_value, k_b_value)));

      ::EndPaint(hwnd, &paint_struct);
    }
    return ::DefWindowProc(hwnd, msg, wparam, lparam);
  }

  static std::wstring utf8_to_unicode(const std::string &utf8) {
  if (utf8.empty()) {
    return std::wstring();
  }

  int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
  wchar_t *buffer = new wchar_t[len];
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buffer, len);
  std::wstring wstr(buffer);
  delete[] buffer;
  return wstr;
}

public:
  simple_window_win(const std::string &title, uint32_t width, uint32_t height, uint64_t style) {
    title_ = title;
    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(&window_proc), &window_instance_);

    WNDCLASSEXW wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.hInstance = window_instance_;
    wcex.lpfnWndProc = &window_proc;
    wcex.lpszClassName = k_window_class_name.c_str();
    window_class_ = ::RegisterClassExW(&wcex);

    uint32_t window_width = width <= 0 ? k_default_width : width;
    uint32_t window_height = height <= 0 ? k_default_height : height;

    std::wstring title_wstr = utf8_to_unicode(title_);
    source_id_ = reinterpret_cast<intptr_t>(
        ::CreateWindowExW(style, k_window_class_name.c_str(), title_wstr.c_str(), WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                          /*parent_window=*/nullptr,
                          /*menu_bar=*/nullptr, window_instance_,
                          /*additional_params=*/nullptr));

    // view_id_ is the same as source_id_ for windows
    view_id_ = source_id_;
  }

  ~simple_window_win() {
    ::DestroyWindow(reinterpret_cast<HWND>(source_id_));
    ::UnregisterClass(MAKEINTATOM(window_class_), window_instance_);
  }

  void minimized() override { ::ShowWindow(reinterpret_cast<HWND>(source_id_), SW_MINIMIZE); }

  void unminimized() override { ::OpenIcon(reinterpret_cast<HWND>(source_id_)); }

  void resize(uint32_t width, uint32_t height) override {
    ::SetWindowPos(reinterpret_cast<HWND>(source_id_), HWND_TOP, 0, 0, width, height,
                   SWP_NOMOVE | SWP_SHOWWINDOW);
    ::UpdateWindow(reinterpret_cast<HWND>(source_id_));
  }

  void move(uint32_t x, uint32_t y) override {
    ::SetWindowPos(reinterpret_cast<HWND>(source_id_), HWND_TOP, x, y, 0, 0,
                   SWP_NOSIZE | SWP_SHOWWINDOW);
    ::UpdateWindow(reinterpret_cast<HWND>(source_id_));
  }

private:
  HINSTANCE window_instance_;
  ATOM window_class_;
};

std::shared_ptr<simple_window> simple_window::create(const std::string &title, uint32_t width,
                                                     uint32_t height, uint64_t style) {
  return std::make_shared<simple_window_win>(title, width, height, style);
}

} // namespace base
} // namespace traa