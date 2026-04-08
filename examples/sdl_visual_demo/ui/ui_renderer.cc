#include "ui/ui_renderer.h"

#include "ui/theme.h"

#include <algorithm>
#include <cstring>

namespace ui {

void draw_text(SDL_Renderer *r, float x, float y, const char *text,
               SDL_Color color) {
  SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
  SDL_RenderDebugText(r, x, y, text);
}

bool draw_button(SDL_Renderer *r, const SDL_FRect &rect, const char *label,
                 bool hovered, bool active) {
  // choose fill color based on state
  SDL_Color fill = theme::button;
  if (active) {
    fill = theme::highlight;
  } else if (hovered) {
    fill = theme::button_hover;
  }

  SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
  SDL_RenderFillRect(r, &rect);

  // center the label text within the button rect
  // SDL_RenderDebugText uses 8x8 pixel characters
  int label_len = label ? static_cast<int>(std::strlen(label)) : 0;
  float text_w = static_cast<float>(label_len * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
  float text_h = static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
  float text_x = rect.x + (rect.w - text_w) * 0.5f;
  float text_y = rect.y + (rect.h - text_h) * 0.5f;

  draw_text(r, text_x, text_y, label ? label : "", theme::text_light);

  // click detection is done externally via hit_test
  return false;
}

void draw_list(SDL_Renderer *r, const SDL_FRect &area, list_state &state,
               int item_count, float item_height,
               void (*draw_item)(SDL_Renderer *r, const SDL_FRect &item_rect,
                                 int index, bool selected, void *userdata),
               void *userdata) {
  // clamp scroll_offset bounds
  float total_height = static_cast<float>(item_count) * item_height;
  float max_scroll = total_height - area.h;
  if (max_scroll < 0.0f) {
    max_scroll = 0.0f;
  }
  state.scroll_offset = std::clamp(state.scroll_offset, 0.0f, max_scroll);

  // set clip rect to the list area
  SDL_Rect clip_rect;
  clip_rect.x = static_cast<int>(area.x);
  clip_rect.y = static_cast<int>(area.y);
  clip_rect.w = static_cast<int>(area.w);
  clip_rect.h = static_cast<int>(area.h);
  SDL_SetRenderClipRect(r, &clip_rect);

  // determine visible item range
  int first_visible = static_cast<int>(state.scroll_offset / item_height);
  if (first_visible < 0) {
    first_visible = 0;
  }
  int visible_count =
      static_cast<int>(area.h / item_height) + 2; // +2 for partial items
  int last_visible = first_visible + visible_count;
  if (last_visible > item_count) {
    last_visible = item_count;
  }

  // draw visible items
  for (int i = first_visible; i < last_visible; ++i) {
    SDL_FRect item_rect;
    item_rect.x = area.x;
    item_rect.y = area.y + static_cast<float>(i) * item_height -
                  state.scroll_offset;
    item_rect.w = area.w;
    item_rect.h = item_height;

    bool selected = (i == state.selected_index);
    draw_item(r, item_rect, i, selected, userdata);
  }

  // clear clip rect
  SDL_SetRenderClipRect(r, nullptr);
}

bool hit_test(const SDL_FRect &rect, float x, float y) {
  return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
         y < rect.y + rect.h;
}

SDL_FRect calc_sidebar_rect(int window_w, int window_h) {
  (void)window_w;
  return {0.0f, 0.0f, static_cast<float>(theme::sidebar_width),
          static_cast<float>(window_h - theme::status_bar_height)};
}

SDL_FRect calc_preview_rect(int window_w, int window_h) {
  return {static_cast<float>(theme::sidebar_width), 0.0f,
          static_cast<float>(window_w - theme::sidebar_width),
          static_cast<float>(window_h - theme::status_bar_height)};
}

SDL_FRect calc_status_bar_rect(int window_w, int window_h) {
  return {0.0f, static_cast<float>(window_h - theme::status_bar_height),
          static_cast<float>(window_w),
          static_cast<float>(theme::status_bar_height)};
}

} // namespace ui
