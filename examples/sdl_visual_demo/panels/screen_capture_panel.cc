#include "screen_capture_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"

#include <traa/traa.h>

#include <cstdio>
#include <cstring>

screen_capture_panel::~screen_capture_panel() {
  if (capturing_) {
    stop_capture();
  }
  destroy_preview_texture();
}

const char *screen_capture_panel::get_name() const { return "Screen Capture"; }

void screen_capture_panel::on_enter() { refresh_source_info(); }

void screen_capture_panel::on_leave() {
  if (capturing_) {
    stop_capture();
  }
}

void screen_capture_panel::refresh_source_info() {
  if (!app_) {
    return;
  }

  current_source_id_ = app_->selected_source_id();
  const char *title = app_->selected_source_title();
  if (title) {
    strncpy(current_source_title_, title, sizeof(current_source_title_) - 1);
    current_source_title_[sizeof(current_source_title_) - 1] = '\0';
  } else {
    current_source_title_[0] = '\0';
  }

  if (app_) {
    if (current_source_id_ != TRAA_INVALID_SCREEN_ID) {
      char buf[512];
      snprintf(buf, sizeof(buf), "Source refreshed: %s (ID: %lld)",
               current_source_title_,
               static_cast<long long>(current_source_id_));
      app_->set_status(buf);
    } else {
      app_->set_status("No source selected");
    }
  }
}

void screen_capture_panel::start_capture() {
  if (capturing_) {
    return;
  }

  traa_screen_capture_config config;
  config.source_id = current_source_id_;
  config.frame_size = traa_size(0, 0);
  config.on_video_frame = on_video_frame;
  config.userdata = this;

  int ret = traa_start_screen_capture(&config);
  if (ret != TRAA_ERROR_NONE) {
    if (app_) {
      char buf[256];
      snprintf(buf, sizeof(buf), "Start screen capture failed: error %d", ret);
      app_->set_status(buf);
    }
    return;
  }

  capturing_ = true;
  frame_count_ = 0;
  fps_last_time_ = SDL_GetTicks();
  current_fps_ = 0.0f;

  if (app_) {
    app_->set_status("Screen capture started");
  }
}

void screen_capture_panel::stop_capture() {
  if (!capturing_) {
    return;
  }

  traa_stop_screen_capture(current_source_id_);

  capturing_ = false;
  destroy_preview_texture();

  if (app_) {
    app_->set_status("Screen capture stopped");
  }
}

void screen_capture_panel::on_video_frame(const traa_userdata userdata,
                                          const traa_video_frame *frame) {
  auto *self = static_cast<screen_capture_panel *>(userdata);
  if (self && frame) {
    self->frame_buf_.push(frame);
  }
}

void screen_capture_panel::on_update() {
  if (!capturing_ || !app_) {
    return;
  }

  // try to get new frame
  SDL_Texture *new_tex = nullptr;
  int w = 0, h = 0;
  if (frame_buf_.pop_to_texture(app_->renderer(), &new_tex, &w, &h)) {
    destroy_preview_texture();
    preview_texture_ = new_tex;
    preview_w_ = w;
    preview_h_ = h;

    // update FPS counter
    ++frame_count_;
    uint64_t now = SDL_GetTicks();
    uint64_t elapsed = now - fps_last_time_;
    if (elapsed >= 1000) {
      current_fps_ =
          static_cast<float>(frame_count_) * 1000.0f /
          static_cast<float>(elapsed);
      frame_count_ = 0;
      fps_last_time_ = now;
    }
  }
}

void screen_capture_panel::on_render(SDL_Renderer *renderer,
                                     const SDL_FRect &area) {
  float x = area.x + k_padding;
  float y = area.y + k_padding;

  // source info
  if (current_source_id_ != TRAA_INVALID_SCREEN_ID) {
    char info[512];
    snprintf(info, sizeof(info), "Source: %s (ID: %lld)",
             current_source_title_,
             static_cast<long long>(current_source_id_));
    ui::draw_text(renderer, x, y, info, theme::text_light);
  } else {
    ui::draw_text(renderer, x, y,
                  "No source selected - go to Sources panel first",
                  theme::error_red);
  }
  y += k_line_height + k_gap;

  // buttons row
  refresh_btn_rect_ = {x, y, k_btn_w, k_btn_h};
  ui::draw_button(renderer, refresh_btn_rect_, "Refresh",
                  refresh_btn_hovered_, false);

  float bx = x + k_btn_w + k_gap;
  start_btn_rect_ = {bx, y, k_btn_w, k_btn_h};
  stop_btn_rect_ = {bx + k_btn_w + k_gap, y, k_btn_w, k_btn_h};

  bool source_valid = (current_source_id_ != TRAA_INVALID_SCREEN_ID);

  if (!capturing_) {
    if (source_valid) {
      ui::draw_button(renderer, start_btn_rect_, "Start",
                      start_btn_hovered_, false);
    } else {
      SDL_SetRenderDrawColor(renderer, theme::separator.r, theme::separator.g,
                             theme::separator.b, theme::separator.a);
      SDL_RenderFillRect(renderer, &start_btn_rect_);
      ui::draw_text(renderer, start_btn_rect_.x + 20.0f,
                    start_btn_rect_.y + 8.0f, "Start", theme::text_light);
    }
  } else {
    ui::draw_button(renderer, stop_btn_rect_, "Stop",
                    stop_btn_hovered_, false);
  }
  y += k_btn_h + k_gap;

  // if capturing, show preview
  if (capturing_ && preview_texture_) {
    // FPS and resolution info
    char info[128];
    snprintf(info, sizeof(info), "FPS: %.1f  Resolution: %dx%d",
             current_fps_, preview_w_, preview_h_);
    ui::draw_text(renderer, x, y, info, theme::success_green);
    y += k_line_height + k_gap;

    // render preview, fit in remaining area
    float avail_w = area.w - k_padding * 2.0f;
    float avail_h = area.y + area.h - y - k_padding;
    if (avail_w > 0.0f && avail_h > 0.0f && preview_w_ > 0 &&
        preview_h_ > 0) {
      float scale_x = avail_w / static_cast<float>(preview_w_);
      float scale_y = avail_h / static_cast<float>(preview_h_);
      float scale = (scale_x < scale_y) ? scale_x : scale_y;
      if (scale > 1.0f) {
        scale = 1.0f;
      }

      float dw = static_cast<float>(preview_w_) * scale;
      float dh = static_cast<float>(preview_h_) * scale;
      SDL_FRect dst = {x, y, dw, dh};
      SDL_RenderTexture(renderer, preview_texture_, nullptr, &dst);
    }
  }
}

bool screen_capture_panel::on_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    refresh_btn_hovered_ =
        ui::hit_test(refresh_btn_rect_, event.motion.x, event.motion.y);
    start_btn_hovered_ =
        ui::hit_test(start_btn_rect_, event.motion.x, event.motion.y);
    stop_btn_hovered_ =
        ui::hit_test(stop_btn_rect_, event.motion.x, event.motion.y);
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    float mx = event.button.x;
    float my = event.button.y;

    if (ui::hit_test(refresh_btn_rect_, mx, my)) {
      if (capturing_) {
        stop_capture();
      }
      refresh_source_info();
      return true;
    }

    bool source_valid = (current_source_id_ != TRAA_INVALID_SCREEN_ID);

    if (!capturing_ && source_valid &&
        ui::hit_test(start_btn_rect_, mx, my)) {
      start_capture();
      return true;
    }

    if (capturing_ && ui::hit_test(stop_btn_rect_, mx, my)) {
      stop_capture();
      return true;
    }
  }

  return false;
}

void screen_capture_panel::destroy_preview_texture() {
  if (preview_texture_) {
    SDL_DestroyTexture(preview_texture_);
    preview_texture_ = nullptr;
  }
  preview_w_ = 0;
  preview_h_ = 0;
}
