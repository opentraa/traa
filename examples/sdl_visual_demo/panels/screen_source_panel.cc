#include "screen_source_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"
#include "../utils/frame_converter.h"

#include <traa/traa.h>

#include <cstdio>
#include <cstring>

screen_source_panel::~screen_source_panel() { destroy_thumbnails(); }

const char *screen_source_panel::get_name() const { return "Sources"; }

void screen_source_panel::on_enter() { enumerate_sources(); }

void screen_source_panel::on_leave() { destroy_thumbnails(); }

void screen_source_panel::destroy_thumbnails() {
  for (auto &s : sources_) {
    if (s.thumbnail) {
      SDL_DestroyTexture(s.thumbnail);
      s.thumbnail = nullptr;
    }
  }
  sources_.clear();
  selected_index_ = -1;
  scroll_offset_ = 0.0f;
}

void screen_source_panel::enumerate_sources() {
  // destroy old data first
  destroy_thumbnails();

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) && \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                     \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

  traa_screen_source_info *infos = nullptr;
  int count = 0;
  traa_size icon_size(0, 0);
  traa_size thumbnail_size(160, 120);

  int ret = traa_enum_screen_source_info(icon_size, thumbnail_size,
                                         TRAA_SCREEN_SOURCE_FLAG_NONE,
                                         &infos, &count);
  if (ret != TRAA_ERROR_NONE) {
    if (app_) {
      char buf[256];
      snprintf(buf, sizeof(buf),
               "enum screen sources failed: error %d", ret);
      app_->set_status(buf);
    }
    return;
  }

  SDL_Renderer *r = app_ ? app_->renderer() : nullptr;

  for (int i = 0; i < count; ++i) {
    source_entry entry;
    entry.id = infos[i].id;
    entry.screen_id = infos[i].screen_id;
    entry.is_window = infos[i].is_window;
    strncpy(entry.title, infos[i].title, sizeof(entry.title) - 1);
    entry.title[sizeof(entry.title) - 1] = '\0';

    // convert thumbnail to SDL_Texture
    if (r && infos[i].thumbnail_data &&
        infos[i].thumbnail_size.width > 0 &&
        infos[i].thumbnail_size.height > 0) {
      entry.thumbnail = frame_converter::bgra_to_texture(
          r, infos[i].thumbnail_data,
          infos[i].thumbnail_size.width,
          infos[i].thumbnail_size.height);
      entry.thumb_w = infos[i].thumbnail_size.width;
      entry.thumb_h = infos[i].thumbnail_size.height;
    }

    sources_.push_back(entry);
  }

  traa_free_screen_source_info(infos, count);

  if (app_) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Found %d screen sources", count);
    app_->set_status(buf);
  }

#else
  if (app_) {
    app_->set_status("Screen source enumeration not supported on this platform");
  }
#endif
}

void screen_source_panel::on_render(SDL_Renderer *renderer,
                                    const SDL_FRect &area) {
  float x = area.x + k_padding;
  float y = area.y + k_padding;

  // draw refresh button
  refresh_btn_rect_ = {x, y, k_btn_w, k_btn_h};
  ui::draw_button(renderer, refresh_btn_rect_, "Refresh",
                  refresh_btn_hovered_, false);
  y += k_btn_h + k_section_gap;

  if (sources_.empty()) {
    ui::draw_text(renderer, x, y, "(no sources found)", theme::text_light);
    return;
  }

  // clip to remaining area
  SDL_FRect list_area = {area.x, y, area.w, area.y + area.h - y};

  // draw each source entry
  float item_y = y - scroll_offset_;
  for (int i = 0; i < static_cast<int>(sources_.size()); ++i) {
    const auto &src = sources_[i];

    // skip items above visible area
    if (item_y + k_item_height < y) {
      item_y += k_item_height;
      continue;
    }
    // stop if below visible area
    if (item_y > area.y + area.h) {
      break;
    }

    // highlight selected item
    if (i == selected_index_) {
      SDL_FRect sel_rect = {area.x, item_y, area.w, k_item_height};
      SDL_SetRenderDrawColor(renderer, theme::highlight.r, theme::highlight.g,
                             theme::highlight.b, 0x30);
      SDL_RenderFillRect(renderer, &sel_rect);
    }

    // draw thumbnail
    if (src.thumbnail) {
      // scale thumbnail to fit within thumb_max bounds
      float scale_x = k_thumb_max_w / static_cast<float>(src.thumb_w);
      float scale_y = k_thumb_max_h / static_cast<float>(src.thumb_h);
      float scale = (scale_x < scale_y) ? scale_x : scale_y;
      if (scale > 1.0f)
        scale = 1.0f;

      float draw_w = static_cast<float>(src.thumb_w) * scale;
      float draw_h = static_cast<float>(src.thumb_h) * scale;
      SDL_FRect dst = {x, item_y + (k_item_height - draw_h) * 0.5f, draw_w,
                       draw_h};
      SDL_RenderTexture(renderer, src.thumbnail, nullptr, &dst);
    } else {
      // placeholder box
      SDL_FRect placeholder = {x, item_y + 10.0f, k_thumb_max_w, k_thumb_max_h};
      SDL_SetRenderDrawColor(renderer, theme::button.r, theme::button.g,
                             theme::button.b, theme::button.a);
      SDL_RenderFillRect(renderer, &placeholder);
      ui::draw_text(renderer, x + 20.0f,
                    item_y + 10.0f + k_thumb_max_h * 0.5f - 4.0f,
                    "(no thumbnail)", theme::text_light);
    }

    // draw text info to the right of thumbnail
    float tx = x + k_text_indent;
    float ty = item_y + 10.0f;

    // title
    ui::draw_text(renderer, tx, ty, src.title, theme::text_light);
    ty += k_line_height + 2.0f;

    // source ID
    char id_buf[64];
    snprintf(id_buf, sizeof(id_buf), "ID: %lld",
             static_cast<long long>(src.id));
    ui::draw_text(renderer, tx, ty, id_buf, theme::text_light);
    ty += k_line_height + 2.0f;

    // type
    const char *type_str = src.is_window ? "Window" : "Screen";
    ui::draw_text(renderer, tx, ty, type_str, theme::highlight);
    ty += k_line_height + 2.0f;

    // screen id (for windows)
    if (src.is_window) {
      char screen_buf[64];
      snprintf(screen_buf, sizeof(screen_buf), "Screen ID: %lld",
               static_cast<long long>(src.screen_id));
      ui::draw_text(renderer, tx, ty, screen_buf, theme::text_light);
    }

    // separator line
    float sep_y = item_y + k_item_height - 1.0f;
    SDL_SetRenderDrawColor(renderer, theme::separator.r, theme::separator.g,
                           theme::separator.b, 0x80);
    SDL_RenderLine(renderer, area.x + k_padding, sep_y,
                   area.x + area.w - k_padding, sep_y);

    item_y += k_item_height;
  }
}

bool screen_source_panel::on_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    refresh_btn_hovered_ =
        ui::hit_test(refresh_btn_rect_, event.motion.x, event.motion.y);
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    // check refresh button
    if (ui::hit_test(refresh_btn_rect_, event.button.x, event.button.y)) {
      enumerate_sources();
      return true;
    }

    // check source item clicks
    if (!sources_.empty()) {
      float list_start_y =
          refresh_btn_rect_.y + k_btn_h + k_section_gap;
      float click_y = event.button.y + scroll_offset_ - list_start_y;

      if (click_y >= 0.0f) {
        int index = static_cast<int>(click_y / k_item_height);
        if (index >= 0 && index < static_cast<int>(sources_.size())) {
          selected_index_ = index;
          if (app_) {
            app_->set_selected_source_id(sources_[index].id);
            app_->set_selected_source_title(sources_[index].title);

            char buf[256];
            snprintf(buf, sizeof(buf), "Selected: %s (ID: %lld)",
                     sources_[index].title,
                     static_cast<long long>(sources_[index].id));
            app_->set_status(buf);
          }
          return true;
        }
      }
    }
  }

  // handle mouse wheel for scrolling
  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    constexpr float scroll_speed = 30.0f;
    scroll_offset_ -= event.wheel.y * scroll_speed;
    if (scroll_offset_ < 0.0f) {
      scroll_offset_ = 0.0f;
    }
    float max_scroll =
        static_cast<float>(sources_.size()) * k_item_height - 400.0f;
    if (max_scroll < 0.0f)
      max_scroll = 0.0f;
    if (scroll_offset_ > max_scroll) {
      scroll_offset_ = max_scroll;
    }
    return true;
  }

  return false;
}
