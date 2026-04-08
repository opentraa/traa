#include "device_panel.h"

#include "../app.h"
#include "../ui/theme.h"
#include "../ui/ui_renderer.h"

#include <traa/traa.h>

#include <cstdio>
#include <cstring>

const char *device_panel::get_name() const { return "Devices"; }

void device_panel::on_enter() { enumerate_devices(); }

void device_panel::enumerate_devices() {
  // clear previous results
  cameras_.clear();
  microphones_.clear();
  speakers_.clear();

  struct type_target {
    traa_device_type type;
    std::vector<device_entry> *dest;
    const char *label;
  };

  type_target targets[] = {
      {TRAA_DEVICE_TYPE_CAMERA, &cameras_, "camera"},
      {TRAA_DEVICE_TYPE_MICROPHONE, &microphones_, "microphone"},
      {TRAA_DEVICE_TYPE_SPEAKER, &speakers_, "speaker"},
  };

  for (auto &t : targets) {
    traa_device_info *infos = nullptr;
    int count = 0;
    int ret = traa_enum_device_info(t.type, &infos, &count);
    if (ret != TRAA_ERROR_NONE) {
      if (app_) {
        char buf[256];
        snprintf(buf, sizeof(buf), "enum %s devices failed: error %d",
                 t.label, ret);
        app_->set_status(buf);
      }
      continue;
    }

    // copy results to internal vectors
    for (int i = 0; i < count; ++i) {
      device_entry entry;
      strncpy(entry.name, infos[i].name, sizeof(entry.name) - 1);
      entry.name[sizeof(entry.name) - 1] = '\0';
      strncpy(entry.id, infos[i].id, sizeof(entry.id) - 1);
      entry.id[sizeof(entry.id) - 1] = '\0';
      t.dest->push_back(entry);
    }

    // free traa-allocated memory
    traa_free_device_info(infos);
  }
}

void device_panel::on_render(SDL_Renderer *renderer, const SDL_FRect &area) {
  constexpr float padding = 10.0f;
  constexpr float line_height = 14.0f;
  constexpr float section_gap = 8.0f;
  constexpr float btn_w = 100.0f;
  constexpr float btn_h = 28.0f;

  float x = area.x + padding;
  float y = area.y + padding;

  // draw refresh button
  refresh_btn_rect_ = {x, y, btn_w, btn_h};
  refresh_btn_hovered_ = ui::hit_test(refresh_btn_rect_, 0, 0); // updated in on_event
  ui::draw_button(renderer, refresh_btn_rect_, "Refresh",
                  refresh_btn_hovered_, false);
  y += btn_h + section_gap;

  // helper lambda to draw a device section
  auto draw_section = [&](const char *title,
                          const std::vector<device_entry> &devices) {
    // section header
    ui::draw_text(renderer, x, y, title, theme::highlight);
    y += line_height + 4.0f;

    if (devices.empty()) {
      ui::draw_text(renderer, x + 10.0f, y, "(none)", theme::text_light);
      y += line_height;
    } else {
      for (const auto &dev : devices) {
        // device name
        ui::draw_text(renderer, x + 10.0f, y, dev.name, theme::text_light);
        y += line_height;
        // device id (indented further)
        char id_line[300];
        snprintf(id_line, sizeof(id_line), "ID: %s", dev.id);
        ui::draw_text(renderer, x + 20.0f, y, id_line, theme::text_light);
        y += line_height;
      }
    }
    y += section_gap;
  };

  draw_section("Cameras", cameras_);
  draw_section("Microphones", microphones_);
  draw_section("Speakers", speakers_);
}

bool device_panel::on_event(const SDL_Event &event) {
  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    refresh_btn_hovered_ =
        ui::hit_test(refresh_btn_rect_, event.motion.x, event.motion.y);
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event.button.button == SDL_BUTTON_LEFT) {
    if (ui::hit_test(refresh_btn_rect_, event.button.x, event.button.y)) {
      enumerate_devices();
      if (app_) {
        app_->set_status("Devices refreshed");
      }
      return true;
    }
  }
  return false;
}
