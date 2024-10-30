#ifndef TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ERROR_TRAP_H_
#define TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ERROR_TRAP_H_

#include <X11/Xlib.h>

#include <mutex>

namespace traa {
namespace base {

// Helper class that registers an X Window error handler. Caller can use
// get_last_error_and_disable() to get the last error that was caught, if any.
class x_error_trap {
public:
  explicit x_error_trap(Display *display);

  x_error_trap(const x_error_trap &) = delete;
  x_error_trap &operator=(const x_error_trap &) = delete;

  ~x_error_trap();

  // Returns the last error if one was caught, otherwise 0. Also unregisters the
  // error handler and replaces it with `original_error_handler_`.
  int get_last_error_and_disable();

private:
  std::unique_lock<std::mutex> mutex_lock_;
  XErrorHandler original_error_handler_ = nullptr;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_LINUX_X11_X_ERROR_TRAP_H_