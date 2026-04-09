#ifndef TRAA_SDL_DEMO_PANELS_SCREEN_CAPTURE_PANEL_H_
#define TRAA_SDL_DEMO_PANELS_SCREEN_CAPTURE_PANEL_H_

#include "panel_base.h"
#include "../utils/frame_buffer.h"

#include <SDL3/SDL.h>
#include <traa/base.h>

#include <cstdint>

class screen_capture_panel : public panel_base {
  // Layout constants shared between on_render and on_event
  static constexpr float k_padding = 10.0f;
  static constexpr float k_btn_w = 100.0f;
  static constexpr float k_btn_h = 28.0f;
  static constexpr float k_line_height = 14.0f;
  static constexpr float k_gap = 6.0f;

public:
  ~screen_capture_panel() override;

  const char *get_name() const override;
  void on_enter() override;
  void on_leave() override;
  void on_update() override;
  void on_render(SDL_Renderer *renderer, const SDL_FRect &area) override;
  bool on_event(const SDL_Event &event) override;

private:
  void start_capture();
  void stop_capture();
  void refresh_source_info();
  void destroy_preview_texture();

  static void on_video_frame(const traa_userdata userdata,
                             const traa_video_frame *frame);

  // Selected source info (read from app)
  int64_t current_source_id_ = TRAA_INVALID_SCREEN_ID;
  char current_source_title_[256] = {};

  // Capture state
  bool capturing_ = false;
  frame_buffer frame_buf_;
  SDL_Texture *preview_texture_ = nullptr;
  int preview_w_ = 0;
  int preview_h_ = 0;

  // FPS tracking
  int frame_count_ = 0;
  uint64_t fps_last_time_ = 0;
  float current_fps_ = 0.0f;

  // Button rects and hover states
  SDL_FRect refresh_btn_rect_ = {};
  SDL_FRect start_btn_rect_ = {};
  SDL_FRect stop_btn_rect_ = {};
  bool refresh_btn_hovered_ = false;
  bool start_btn_hovered_ = false;
  bool stop_btn_hovered_ = false;
};

#endif // TRAA_SDL_DEMO_PANELS_SCREEN_CAPTURE_PANEL_H_
