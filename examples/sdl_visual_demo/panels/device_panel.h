#ifndef TRAA_SDL_DEMO_PANELS_DEVICE_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_DEVICE_PANEL_H_

#include "panel_base.h"

#include <traa/base.h>

#include <vector>

class device_panel : public panel_base {
public:
  const char *get_name() const override;
  void on_enter() override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  struct device_entry {
    char name[TRAA_MAX_DEVICE_NAME_LENGTH];
    char id[TRAA_MAX_DEVICE_ID_LENGTH];
  };

  void enumerate_devices();

  std::vector<device_entry> cameras_;
  std::vector<device_entry> microphones_;
  std::vector<device_entry> speakers_;

  // refresh button state
  SDL_FRect refresh_btn_rect_ = {};
  bool refresh_btn_hovered_ = false;
};

#endif // TRAA_SDL_DEMO_PANELS_DEVICE_PANEL_H_
