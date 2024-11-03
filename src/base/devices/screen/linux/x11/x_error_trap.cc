#include "base/devices/screen/linux/x11/x_error_trap.h"

#include "base/log/logger.h"

#include <stddef.h>

#include <atomic>

namespace traa {
namespace base {

inline namespace {

static int g_last_xserver_error_code = 0;
static std::atomic<Display *> g_display_for_error_handler = nullptr;

static std::mutex g_mutex_;

int xserver_error_handler(Display *display, XErrorEvent *error_event) {
  g_last_xserver_error_code = error_event->error_code;
  return 0;
}

} // namespace

x_error_trap::x_error_trap(Display *display) : mutex_lock_(g_mutex_) {
  // We don't expect this class to be used in a nested fashion so therefore
  // g_display_for_error_handler should never be valid here.
  g_display_for_error_handler.store(display);
  g_last_xserver_error_code = 0;
  original_error_handler_ = XSetErrorHandler(&xserver_error_handler);
}

int x_error_trap::get_last_error_and_disable() {
  g_display_for_error_handler.store(nullptr);
  XSetErrorHandler(original_error_handler_);
  if(g_last_xserver_error_code != 0) {
    LOG_ERROR("X11 error code: {}", g_last_xserver_error_code);
  }
  return g_last_xserver_error_code;
}

x_error_trap::~x_error_trap() {
  if (g_display_for_error_handler.load() != nullptr)
    get_last_error_and_disable();
}

} // namespace base
} // namespace traa