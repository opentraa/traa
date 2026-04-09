#include "camera_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"

#include <traa/traa.h>

#include <cstdio>
#include <cstring>

camera_panel::~camera_panel() {
  stop_capture();
  destroy_preview_texture();
}

const char *camera_panel::get_name() const { return "Camera"; }

void camera_panel::on_enter() { enumerate_cameras(); }

void camera_panel::on_leave() { stop_capture(); }

void camera_panel::destroy_preview_texture() {
  if (preview_texture_) {
    SDL_DestroyTexture(preview_texture_);
    preview_texture_ = nullptr;
  }
}

void camera_panel::enumerate_cameras() {
  cameras_.clear();
  selected_device_ = -1;
  capabilities_.clear();
  selected_cap_ = -1;

  traa_device_info *infos = nullptr;
  int count = 0;
  int ret = traa_enum_device_info(TRAA_DEVICE_TYPE_CAMERA, &infos, &count);
  if (ret != TRAA_ERROR_NONE) {
    if (app_) {
      char buf[256];
      snprintf(buf, sizeof(buf), "Enum cameras failed: error %d", ret);
      app_->set_status(buf);
    }
    return;
  }

  for (int i = 0; i < count; ++i) {
    device_entry entry;
    strncpy(entry.name, infos[i].name, sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
    strncpy(entry.id, infos[i].id, sizeof(entry.id) - 1);
    entry.id[sizeof(entry.id) - 1] = '\0';
    cameras_.push_back(entry);
  }

  traa_free_device_info(infos);

  if (app_) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Found %d cameras", count);
    app_->set_status(buf);
  }
}

void camera_panel::query_capabilities() {
  capabilities_.clear();
  selected_cap_ = -1;

  if (selected_device_ < 0 ||
      selected_device_ >= static_cast<int>(cameras_.size())) {
    return;
  }

  traa_video_capability *caps = nullptr;
  int count = 0;
  int ret = traa_get_camera_capability(cameras_[selected_device_].id,
                                       &caps, &count);
  if (ret != TRAA_ERROR_NONE) {
    if (app_) {
      char buf[256];
      snprintf(buf, sizeof(buf), "Get capabilities failed: error %d", ret);
      app_->set_status(buf);
    }
    return;
  }

  for (int i = 0; i < count; ++i) {
    capabilities_.push_back(caps[i]);
  }

  traa_free_camera_capability(caps);

  if (app_) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %d capabilities",
             cameras_[selected_device_].name, count);
    app_->set_status(buf);
  }
}

void camera_panel::start_capture() {
  if (capturing_) return;
  if (selected_device_ < 0 || selected_cap_ < 0) return;

  traa_camera_config config;
  config.device_id = cameras_[selected_device_].id;
  config.capability = capabilities_[selected_cap_];
  config.on_video_frame = on_video_frame;
  config.userdata = this;

  int ret = traa_start_camera_capture(&config);
  if (ret != TRAA_ERROR_NONE) {
    if (app_) {
      char buf[256];
      snprintf(buf, sizeof(buf), "Start capture failed: error %d", ret);
      app_->set_status(buf);
    }
    return;
  }

  capturing_ = true;
  frame_count_ = 0;
  fps_last_time_ = SDL_GetTicks();
  current_fps_ = 0.0f;

  if (app_) {
    app_->set_status("Camera capture started");
  }
}

void camera_panel::stop_capture() {
  if (!capturing_) return;

  if (selected_device_ >= 0 &&
      selected_device_ < static_cast<int>(cameras_.size())) {
    traa_stop_camera_capture(cameras_[selected_device_].id);
  }

  capturing_ = false;
  destroy_preview_texture();

  if (app_) {
    app_->set_status("Camera capture stopped");
  }
}

void camera_panel::on_video_frame(const traa_userdata userdata,
                                  const traa_video_frame *frame) {
  auto *self = static_cast<camera_panel *>(userdata);
  if (self && frame) {
    self->frame_buf_.push(frame);
  }
}

void camera_panel::on_update() {
  if (!capturing_ || !app_) return;

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

void camera_panel::on_render(SDL_Renderer *renderer, const SDL_FRect &area) {
  float x = area.x + k_padding;
  float y = area.y + k_padding;

  // refresh button
  refresh_btn_rect_ = {x, y, k_btn_w, k_btn_h};
  ui::draw_button(renderer, refresh_btn_rect_, "Refresh",
                  refresh_btn_hovered_, false);

  // start/stop buttons
  float bx = x + k_btn_w + k_gap;
  start_btn_rect_ = {bx, y, k_btn_w, k_btn_h};
  stop_btn_rect_ = {bx + k_btn_w + k_gap, y, k_btn_w, k_btn_h};

  if (!capturing_) {
    bool can_start = selected_device_ >= 0 && selected_cap_ >= 0;
    if (can_start) {
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
      if (scale > 1.0f) scale = 1.0f;

      float dw = static_cast<float>(preview_w_) * scale;
      float dh = static_cast<float>(preview_h_) * scale;
      SDL_FRect dst = {x, y, dw, dh};
      SDL_RenderTexture(renderer, preview_texture_, nullptr, &dst);
    }
    return;
  }

  // device list
  ui::draw_text(renderer, x, y, "Cameras:", theme::highlight);
  y += k_line_height + 2.0f;

  if (cameras_.empty()) {
    ui::draw_text(renderer, x + 10.0f, y, "(none)", theme::text_light);
    y += k_item_h;
  } else {
    for (int i = 0; i < static_cast<int>(cameras_.size()); ++i) {
      bool selected = (i == selected_device_);
      SDL_Color color = selected ? theme::highlight : theme::text_light;
      char label[300];
      snprintf(label, sizeof(label), "%s%s", selected ? "> " : "  ",
               cameras_[i].name);
      ui::draw_text(renderer, x + 10.0f, y, label, color);
      y += k_item_h;
    }
  }
  y += k_gap;

  // capability list
  if (selected_device_ >= 0 && !capabilities_.empty()) {
    ui::draw_text(renderer, x, y, "Capabilities:", theme::highlight);
    y += k_line_height + 2.0f;

    for (int i = 0; i < static_cast<int>(capabilities_.size()); ++i) {
      const auto &cap = capabilities_[i];
      bool selected = (i == selected_cap_);
      SDL_Color color = selected ? theme::highlight : theme::text_light;
      char label[128];
      snprintf(label, sizeof(label), "%s%dx%d @%dfps fmt:%d",
               selected ? "> " : "  ", cap.width, cap.height, cap.max_fps,
               static_cast<int>(cap.format));
      ui::draw_text(renderer, x + 10.0f, y, label, color);
      y += k_item_h;
    }
  }
}

bool camera_panel::on_event(const SDL_Event &event) {
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
      stop_capture();
      enumerate_cameras();
      return true;
    }

    if (!capturing_ && ui::hit_test(start_btn_rect_, mx, my)) {
      if (selected_device_ >= 0 && selected_cap_ >= 0) {
        start_capture();
      }
      return true;
    }

    if (capturing_ && ui::hit_test(stop_btn_rect_, mx, my)) {
      stop_capture();
      return true;
    }

    // click on device list
    float list_y = refresh_btn_rect_.y + k_btn_h + k_gap + k_line_height + 2.0f;

    // check device clicks
    for (int i = 0; i < static_cast<int>(cameras_.size()); ++i) {
      float iy = list_y + static_cast<float>(i) * k_item_h;
      SDL_FRect item_rect = {refresh_btn_rect_.x, iy,
                             300.0f, k_item_h};
      if (ui::hit_test(item_rect, mx, my)) {
        if (selected_device_ != i) {
          selected_device_ = i;
          query_capabilities();
        }
        return true;
      }
    }

    // check capability clicks
    if (selected_device_ >= 0 && !capabilities_.empty()) {
      float cap_y = list_y +
                    static_cast<float>(cameras_.size()) * k_item_h + k_gap +
                    k_line_height + 2.0f;
      for (int i = 0; i < static_cast<int>(capabilities_.size()); ++i) {
        float iy = cap_y + static_cast<float>(i) * k_item_h;
        SDL_FRect item_rect = {refresh_btn_rect_.x, iy,
                               300.0f, k_item_h};
        if (ui::hit_test(item_rect, mx, my)) {
          selected_cap_ = i;
          return true;
        }
      }
    }
  }

  return false;
}
