#ifndef TRAA_SDL_DEMO_UI_THEME_H_
#define TRAA_SDL_DEMO_UI_THEME_H_

#include <SDL3/SDL.h>

namespace theme {

// color palette (Catppuccin Mocha inspired)
constexpr SDL_Color bg_dark = {0x1E, 0x1E, 0x2E, 0xFF};       // #1E1E2E
constexpr SDL_Color text_light = {0xCD, 0xD6, 0xF4, 0xFF};     // #CDD6F4
constexpr SDL_Color highlight = {0x89, 0xB4, 0xFA, 0xFF};      // #89B4FA
constexpr SDL_Color button = {0x31, 0x32, 0x44, 0xFF};         // #313244
constexpr SDL_Color button_hover = {0x45, 0x47, 0x5A, 0xFF};   // #45475A
constexpr SDL_Color separator = {0x58, 0x5B, 0x70, 0xFF};      // #585B70
constexpr SDL_Color error_red = {0xF3, 0x8B, 0xA8, 0xFF};      // #F38BA8
constexpr SDL_Color success_green = {0xA6, 0xE3, 0xA1, 0xFF};  // #A6E3A1

// layout constants
constexpr int sidebar_width = 200;
constexpr int status_bar_height = 32;
constexpr int default_window_w = 1280;
constexpr int default_window_h = 720;

} // namespace theme

#endif // TRAA_SDL_DEMO_UI_THEME_H_
