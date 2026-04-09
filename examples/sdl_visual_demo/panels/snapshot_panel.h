#ifndef TRAA_SDL_DEMO_PANELS_SNAPSHOT_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_SNAPSHOT_PANEL_H_

#include "panel_base.h"

#include <SDL3/SDL.h>

#include <cstdint>

class snapshot_panel : public panel_base {
public:
  ~snapshot_panel() override;

  const char *get_name() const override;
  void on_leave() override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  void take_snapshot();
  void destroy_texture();

  SDL_Texture *snapshot_texture_ = nullptr;
  int snap_w_ = 0;
  int snap_h_ = 0;
  int snap_data_size_ = 0;

  // button state
  SDL_FRect snap_btn_rect_ = {};
  bool snap_btn_hovered_ = false;
};

#endif // TRAA_SDL_DEMO_PANELS_SNAPSHOT_PANEL_H_
