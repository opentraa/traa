#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_

#include "base/disallow.h"
#include <traa/base.h>

#include <windows.h>

namespace traa {
namespace base {

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

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_THUMBNAIL_H_