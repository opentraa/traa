#include "base/devices/screen/win/thumbnail.h"

#include "base/devices/screen/win/dwm_helper.h"
#include "base/devices/screen/win/gdi_helper.h"
#include "base/log/logger.h"

namespace traa {
namespace base {

thumbnail::thumbnail() : dwm_window_(nullptr) {
  if (!dwm_helper::is_dwm_supported()) {
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
  if (!dwm_helper::get_thubmnail_data_from_dwm(dwm_window_, window, thumbnail_size, data, size)) {
    return gdi_helper::get_image(window, thumbnail_size, data, size);
  }

  return true;
}

} // namespace base
} // namespace traa