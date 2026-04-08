#ifndef TRAA_SDL_DEMO_PANELS_LOG_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_LOG_PANEL_H_

#include "panel_base.h"

#include <SDL3/SDL.h>
#include <traa/base.h>

class log_panel : public panel_base {
public:
  const char *get_name() const override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  traa_log_level current_level_ = TRAA_LOG_LEVEL_INFO;
  float scroll_offset_ = 0.0f;
  bool auto_scroll_ = true;

  // level button rects
  static constexpr int k_num_levels = 7;
  SDL_FRect level_btn_rects_[k_num_levels] = {};
  bool level_btn_hovered_[k_num_levels] = {};
};

#endif // TRAA_SDL_DEMO_PANELS_LOG_PANEL_H_
