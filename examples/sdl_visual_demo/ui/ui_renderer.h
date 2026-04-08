#ifndef TRAA_SDL_DEMO_UI_UI_RENDERER_H_
#define TRAA_SDL_DEMO_UI_UI_RENDERER_H_

#include <SDL3/SDL.h>

namespace ui {

// draw text using SDL_RenderDebugText (8x8 pixel monospaced characters)
void draw_text(SDL_Renderer *r, float x, float y, const char *text, SDL_Color color);

// draw a button with hover/active highlight; returns false (click detection is
// done externally via hit_test)
bool draw_button(SDL_Renderer *r, const SDL_FRect &rect, const char *label,
                 bool hovered, bool active);

// scrollable list state
struct list_state {
  float scroll_offset = 0.0f;
  int selected_index = -1;
};

// draw a scrollable list; calls draw_item callback for each visible item
void draw_list(SDL_Renderer *r, const SDL_FRect &area, list_state &state,
               int item_count, float item_height,
               void (*draw_item)(SDL_Renderer *r, const SDL_FRect &item_rect,
                                 int index, bool selected, void *userdata),
               void *userdata);

// point-in-rect hit test
bool hit_test(const SDL_FRect &rect, float x, float y);

// layout calculation functions
SDL_FRect calc_sidebar_rect(int window_w, int window_h);
SDL_FRect calc_preview_rect(int window_w, int window_h);
SDL_FRect calc_status_bar_rect(int window_w, int window_h);

} // namespace ui

#endif // TRAA_SDL_DEMO_UI_UI_RENDERER_H_
