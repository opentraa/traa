#ifndef TRAA_SDL_DEMO_PANEL_MANAGER_H_
#define TRAA_SDL_DEMO_PANEL_MANAGER_H_

#include "panels/panel_base.h"

#include <memory>
#include <vector>

class panel_manager {
public:
  void register_panel(std::unique_ptr<panel_base> panel);
  void switch_to(int index);
  void handle_event(const SDL_Event &event);
  void update();
  void render(SDL_Renderer *renderer, const SDL_FRect &area);

  int panel_count() const;
  const char *panel_name(int index) const;
  int active_index() const;

  // set the app pointer on all registered panels
  void set_app(app *a);

private:
  std::vector<std::unique_ptr<panel_base>> panels_;
  int active_index_ = -1;
};

#endif // TRAA_SDL_DEMO_PANEL_MANAGER_H_
