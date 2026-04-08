#include "snapshot_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"
#include "../utils/frame_converter.h"

#include <traa/traa.h>

#include <cstdio>
#include <cstring>

snapshot_panel::~snapshot_panel() { destroy_texture(); }

const char *snapshot_panel::get_name() const { return "Snapshot"; }

void snapshot_panel::on_leave() { destroy_texture(); }

void snapshot_panel::destroy_texture() {
  if (snapshot_texture_) {
    SDL_DestroyTexture(snapshot_texture_);
    snapshot_texture_ = nullptr;
  }
  snap_w_ = 0;
  snap_h_ = 0;
  snap_data_size_ = 0;
}

void snapshot_panel::take_snapshot() {
#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) &&        \
    !defined(__ANDROID__) &&                                                \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                   \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

  if (!app_) return;

  int64_t source_id = app_->selected_source_id();
  if (source_id == TRAA_INVALID_SCREEN_ID) {
    app_->set_status("No source selected");
    return;
  }

  destroy_texture();

  uint8_t *data = nullptr;
  int data_size = 0;
  traa_size actual_size;
  traa_size snapshot_size(0, 0); // 0 means original size

  int ret = traa_create_snapshot(source_id, snapshot_size, &data, &data_size,
                                 &actual_size);
  if (ret != TRAA_ERROR_NONE) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Snapshot failed: error %d", ret);
    app_->set_status(buf);
    return;
  }

  // convert BGRA data to texture
  SDL_Renderer *r = app_->renderer();
  if (r && data && actual_size.width > 0 && actual_size.height > 0) {
    snapshot_texture_ = frame_converter::bgra_to_texture(
        r, data, actual_size.width, actual_size.height);
    snap_w_ = actual_size.width;
    snap_h_ = actual_size.height;
    snap_data_size_ = data_size;
  }

  traa_free_snapshot(data);

  char buf[256];
  snprintf(buf, sizeof(buf), "Snapshot: %dx%d (%d bytes)",
           actual_size.width, actual_size.height, data_size);
  app_->set_status(buf);

#else
  if (app_) {
    app_->set_status("Snapshot not supported on this platform");
  }
#endif
}

void snapshot_panel::on_render(SDL_Renderer *renderer, const SDL_FRect &area) {
  constexpr float padding = 10.0f;
  constexpr float btn_w = 120.0f;
  constexpr float btn_h = 28.0f;
  constexpr float line_height = 14.0f;
  constexpr float section_gap = 8.0f;

  float x = area.x + padding;
  float y = area.y + padding;

  // show selected source info
  if (app_) {
    int64_t sid = app_->selected_source_id();
    if (sid != TRAA_INVALID_SCREEN_ID) {
      char info[300];
      snprintf(info, sizeof(info), "Source: %s (ID: %lld)",
               app_->selected_source_title(),
               static_cast<long long>(sid));
      ui::draw_text(renderer, x, y, info, theme::text_light);
    } else {
      ui::draw_text(renderer, x, y,
                    "No source selected - go to Sources panel first",
                    theme::error_red);
    }
  }
  y += line_height + section_gap;

  // snapshot button
  bool enabled = app_ && app_->selected_source_id() != TRAA_INVALID_SCREEN_ID;
  snap_btn_rect_ = {x, y, btn_w, btn_h};

  if (enabled) {
    ui::draw_button(renderer, snap_btn_rect_, "Take Snapshot",
                    snap_btn_hovered_, false);
  } else {
    // draw disabled button
    SDL_SetRenderDrawColor(renderer, theme::separator.r, theme::separator.g,
                           theme::separator.b, theme::separator.a);
    SDL_RenderFillRect(renderer, &snap_btn_rect_);
    float tx = snap_btn_rect_.x + 10.0f;
    float ty = snap_btn_rect_.y +
               (snap_btn_rect_.h -
                static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) *
                   0.5f;
    ui::draw_text(renderer, tx, ty, "Take Snapshot", theme::text_light);
  }
  y += btn_h + section_gap;

  // show snapshot info
  if (snapshot_texture_) {
    char size_info[128];
    snprintf(size_info, sizeof(size_info), "Size: %dx%d  Data: %d bytes",
             snap_w_, snap_h_, snap_data_size_);
    ui::draw_text(renderer, x, y, size_info, theme::success_green);
    y += line_height + section_gap;

    // render snapshot preview, fit within remaining area
    float avail_w = area.w - padding * 2.0f;
    float avail_h = area.y + area.h - y - padding;
    if (avail_w > 0.0f && avail_h > 0.0f) {
      float scale_x = avail_w / static_cast<float>(snap_w_);
      float scale_y = avail_h / static_cast<float>(snap_h_);
      float scale = (scale_x < scale_y) ? scale_x : scale_y;
      if (scale > 1.0f) scale = 1.0f;

      float draw_w = static_cast<float>(snap_w_) * scale;
      float draw_h = static_cast<float>(snap_h_) * scale;
      SDL_FRect dst = {x, y, draw_w, draw_h};
      SDL_RenderTexture(renderer, snapshot_texture_, nullptr, &dst);
    }
  }
}

bool snapshot_panel::on_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    snap_btn_hovered_ =
        ui::hit_test(snap_btn_rect_, event.motion.x, event.motion.y);
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    if (ui::hit_test(snap_btn_rect_, event.button.x, event.button.y)) {
      bool enabled =
          app_ && app_->selected_source_id() != TRAA_INVALID_SCREEN_ID;
      if (enabled) {
        take_snapshot();
      }
      return true;
    }
  }
  return false;
}
