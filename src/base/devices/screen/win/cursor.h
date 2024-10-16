#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_

#include <windows.h>

namespace traa {
namespace base {

class mouse_cursor;

// Converts an HCURSOR into a `mouse_cursor` instance.
mouse_cursor *create_mouse_cursor_from_handle(HDC dc, HCURSOR cursor);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_CURSOR_H_