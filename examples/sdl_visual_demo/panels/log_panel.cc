#include "log_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"

#include <traa/traa.h>

#include <cstdio>
#include <mutex>

const char *log_panel::get_name() const { return "Log"; }

static const char *level_names[7] = {"TRACE", "DEBUG", "INFO", "WARN",
                                     "ERROR", "FATAL", "OFF"};

void log_panel::on_render(SDL_Renderer *renderer, const SDL_FRect &area) {
  constexpr float padding = 10.0f;
  constexpr float btn_w = 70.0f;
  constexpr float btn_h = 24.0f;
  constexpr float btn_gap = 4.0f;
  constexpr float line_height = 12.0f;
  constexpr float section_gap = 10.0f;

  float x = area.x + padding;
  float y = area.y + padding;

  // draw log level selector
  ui::draw_text(renderer, x, y, "Log Level:", theme::text_light);
  y += line_height + 4.0f;

  float bx = x;
  for (int i = 0; i < k_num_levels; ++i) {
    level_btn_rects_[i] = {bx, y, btn_w, btn_h};
    bool active = (static_cast<int>(current_level_) == i);
    ui::draw_button(renderer, level_btn_rects_[i], level_names[i],
                    level_btn_hovered_[i], active);
    bx += btn_w + btn_gap;
  }
  y += btn_h + section_gap;

  // separator
  SDL_SetRenderDrawColor(renderer, theme::separator.r, theme::separator.g,
                         theme::separator.b, theme::separator.a);
  SDL_RenderLine(renderer, area.x + padding, y,
                 area.x + area.w - padding, y);
  y += 4.0f;

  // log output area
  float log_area_h = area.y + area.h - y - padding;
  if (log_area_h <= 0.0f) return;

  SDL_FRect log_area = {area.x, y, area.w, log_area_h};

  // clip to log area
  SDL_Rect clip;
  clip.x = static_cast<int>(log_area.x);
  clip.y = static_cast<int>(log_area.y);
  clip.w = static_cast<int>(log_area.w);
  clip.h = static_cast<int>(log_area.h);
  SDL_SetRenderClipRect(renderer, &clip);

  if (!app_) {
    SDL_SetRenderClipRect(renderer, nullptr);
    return;
  }

  std::lock_guard<std::mutex> lock(app_->log_mutex());
  const auto &logs = app_->log_buffer();

  float total_h = static_cast<float>(logs.size()) * line_height;

  // auto-scroll to bottom
  if (auto_scroll_ && total_h > log_area_h) {
    scroll_offset_ = total_h - log_area_h;
  }

  float ly = y - scroll_offset_;
  for (const auto &msg : logs) {
    if (ly + line_height >= y && ly < y + log_area_h) {
      ui::draw_text(renderer, x, ly, msg.c_str(), theme::text_light);
    }
    ly += line_height;
  }

  SDL_SetRenderClipRect(renderer, nullptr);
}

bool log_panel::on_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    for (int i = 0; i < k_num_levels; ++i) {
      level_btn_hovered_[i] =
          ui::hit_test(level_btn_rects_[i], event.motion.x, event.motion.y);
    }
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    for (int i = 0; i < k_num_levels; ++i) {
      if (ui::hit_test(level_btn_rects_[i], event.button.x,
                       event.button.y)) {
        current_level_ = static_cast<traa_log_level>(i);
        traa_set_log_level(current_level_);
        if (app_) {
          char buf[64];
          snprintf(buf, sizeof(buf), "Log level set to %s", level_names[i]);
          app_->set_status(buf);
        }
        return true;
      }
    }
  }

  // scroll
  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    constexpr float scroll_speed = 30.0f;
    scroll_offset_ -= event.wheel.y * scroll_speed;
    if (scroll_offset_ < 0.0f) scroll_offset_ = 0.0f;
    auto_scroll_ = false; // user scrolled manually
    return true;
  }

  return false;
}
