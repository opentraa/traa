#ifndef TRAA_SDL_DEMO_PANELS_PANEL_BASE_H_
#define TRAA_SDL_DEMO_PANELS_PANEL_BASE_H_

#include <SDL3/SDL.h>
#include <string>

// forward declaration
class app;

class panel_base {
public:
  virtual ~panel_base() = default;

  // return the panel display name (used for sidebar buttons)
  virtual const char *get_name() const = 0;

  // called when the panel becomes active
  virtual void on_enter() {}

  // called when the panel is deactivated
  virtual void on_leave() {}

  // per-frame update logic
  virtual void on_update() {}

  // render panel content into the specified area
  virtual void on_render(SDL_Renderer *renderer, const SDL_FRect &area) = 0;

  // handle SDL event; return true if the event was consumed
  virtual bool on_event(const SDL_Event &event) { return false; }

protected:
  // panels can access shared app state (e.g. selected source id) through this pointer
  app *app_ = nullptr;
  friend class panel_manager;
};

#endif // TRAA_SDL_DEMO_PANELS_PANEL_BASE_H_
