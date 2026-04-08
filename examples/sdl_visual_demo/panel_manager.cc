#include "panel_manager.h"

void panel_manager::register_panel(std::unique_ptr<panel_base> panel) {
  panels_.push_back(std::move(panel));
  if (panels_.size() == 1) {
    switch_to(0);
  }
}

void panel_manager::switch_to(int index) {
  if (index < 0 || index >= static_cast<int>(panels_.size())) {
    return;
  }
  if (index == active_index_) {
    return;
  }
  if (active_index_ >= 0 && active_index_ < static_cast<int>(panels_.size())) {
    panels_[active_index_]->on_leave();
  }
  active_index_ = index;
  panels_[active_index_]->on_enter();
}

void panel_manager::handle_event(const SDL_Event &event) {
  if (active_index_ >= 0 && active_index_ < static_cast<int>(panels_.size())) {
    panels_[active_index_]->on_event(event);
  }
}

void panel_manager::update() {
  if (active_index_ >= 0 && active_index_ < static_cast<int>(panels_.size())) {
    panels_[active_index_]->on_update();
  }
}

void panel_manager::render(SDL_Renderer *renderer, const SDL_FRect &area) {
  if (active_index_ >= 0 && active_index_ < static_cast<int>(panels_.size())) {
    panels_[active_index_]->on_render(renderer, area);
  }
}

int panel_manager::panel_count() const {
  return static_cast<int>(panels_.size());
}

const char *panel_manager::panel_name(int index) const {
  if (index < 0 || index >= static_cast<int>(panels_.size())) {
    return nullptr;
  }
  return panels_[index]->get_name();
}

int panel_manager::active_index() const {
  return active_index_;
}

void panel_manager::set_app(app *a) {
  for (auto &panel : panels_) {
    panel->app_ = a;
  }
}
