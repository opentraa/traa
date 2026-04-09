#ifndef TRAA_SDL_DEMO_APP_H_
#define TRAA_SDL_DEMO_APP_H_

#include "panel_manager.h"

#include <SDL3/SDL.h>
#include <traa/traa.h>

#include <deque>
#include <mutex>
#include <string>
#include <vector>

class app {
public:
  bool init();
  void run();
  void shutdown();

  // shared state accessors
  int64_t selected_source_id() const;
  void set_selected_source_id(int64_t id);
  const char *selected_source_title() const;
  void set_selected_source_title(const char *title);

  void set_status(const char *text);
  void append_log(const char *message);

  SDL_Renderer *renderer() const;
  panel_manager &panels();

  // log buffer access (for log_panel)
  const std::deque<std::string> &log_buffer() const;
  std::mutex &log_mutex();

private:
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  panel_manager panel_mgr_;
  bool running_ = false;

  // shared state
  int64_t selected_source_id_ = TRAA_INVALID_SCREEN_ID;
  char selected_source_title_[256] = {};
  char status_text_[512] = {};

  // log buffer
  std::deque<std::string> log_buffer_;
  std::mutex log_mutex_;
  static constexpr int k_max_log_entries = 1000;

  // mouse state for UI
  float mouse_x_ = 0.0f;
  float mouse_y_ = 0.0f;
  bool mouse_clicked_ = false;

  void register_panels();
  void render_sidebar(const SDL_FRect &area);
  void render_status_bar(const SDL_FRect &area);

  // traa error callback
  static void on_traa_error(const traa_userdata userdata, traa_error error,
                            const char *message);
};

#endif // TRAA_SDL_DEMO_APP_H_
