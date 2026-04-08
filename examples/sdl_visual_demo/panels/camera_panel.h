#ifndef TRAA_SDL_DEMO_PANELS_CAMERA_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_CAMERA_PANEL_H_

#include "panel_base.h"
#include "../utils/frame_buffer.h"

#include <SDL3/SDL.h>
#include <traa/base.h>

#include <cstdint>
#include <string>
#include <vector>

class camera_panel : public panel_base {
  // Layout constants shared between on_render and on_event
  static constexpr float k_padding = 10.0f;
  static constexpr float k_btn_w = 100.0f;
  static constexpr float k_btn_h = 28.0f;
  static constexpr float k_line_height = 14.0f;
  static constexpr float k_gap = 6.0f;
  static constexpr float k_item_h = 20.0f;

public:
  ~camera_panel() override;

  const char *get_name() const override;
  void on_enter() override;
  void on_leave() override;
  void on_update() override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  struct device_entry {
    char name[TRAA_MAX_DEVICE_NAME_LENGTH];
    char id[TRAA_MAX_DEVICE_ID_LENGTH];
  };

  void enumerate_cameras();
  void query_capabilities();
  void start_capture();
  void stop_capture();
  void destroy_preview_texture();

  static void on_video_frame(const traa_userdata userdata,
                             const traa_video_frame *frame);

  std::vector<device_entry> cameras_;
  int selected_device_ = -1;

  std::vector<traa_video_capability> capabilities_;
  int selected_cap_ = -1;

  bool capturing_ = false;
  frame_buffer frame_buf_;
  SDL_Texture *preview_texture_ = nullptr;
  int preview_w_ = 0;
  int preview_h_ = 0;

  // FPS tracking
  int frame_count_ = 0;
  uint64_t fps_last_time_ = 0;
  float current_fps_ = 0.0f;

  // button rects
  SDL_FRect refresh_btn_rect_ = {};
  SDL_FRect start_btn_rect_ = {};
  SDL_FRect stop_btn_rect_ = {};
  bool refresh_btn_hovered_ = false;
  bool start_btn_hovered_ = false;
  bool stop_btn_hovered_ = false;

  // scroll for device/capability lists
  float scroll_offset_ = 0.0f;
};

#endif // TRAA_SDL_DEMO_PANELS_CAMERA_PANEL_H_
