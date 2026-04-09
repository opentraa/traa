#ifndef TRAA_SDL_DEMO_PANELS_SCREEN_SOURCE_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_SCREEN_SOURCE_PANEL_H_

#include "panel_base.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

class screen_source_panel : public panel_base {
  // Layout constants shared between on_render and on_event
  static constexpr float k_padding = 10.0f;
  static constexpr float k_btn_w = 100.0f;
  static constexpr float k_btn_h = 28.0f;
  static constexpr float k_section_gap = 8.0f;
  static constexpr float k_item_height = 140.0f;
  static constexpr float k_thumb_max_w = 160.0f;
  static constexpr float k_thumb_max_h = 120.0f;
  static constexpr float k_line_height = 14.0f;
  static constexpr float k_text_indent = 170.0f;

public:
  ~screen_source_panel() override;

  const char *get_name() const override;
  void on_enter() override;
  void on_leave() override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  struct source_entry {
    int64_t id = -2;
    int64_t screen_id = -2;
    bool is_window = false;
    char title[256] = {};
    SDL_Texture *thumbnail = nullptr;
    int thumb_w = 0;
    int thumb_h = 0;
  };

  void enumerate_sources();
  void destroy_thumbnails();

  std::vector<source_entry> sources_;
  int selected_index_ = -1;
  float scroll_offset_ = 0.0f;

  // refresh button state
  SDL_FRect refresh_btn_rect_ = {};
  bool refresh_btn_hovered_ = false;
};

#endif // TRAA_SDL_DEMO_PANELS_SCREEN_SOURCE_PANEL_H_
