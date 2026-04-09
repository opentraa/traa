#include "app.h"

#include "panels/device_panel.h"
#include "panels/screen_source_panel.h"
#include "panels/snapshot_panel.h"
#include "panels/camera_panel.h"
#include "panels/screen_capture_panel.h"
#include "panels/log_panel.h"
#include "ui/theme.h"
#include "ui/ui_renderer.h"

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool app::init() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  window_ = SDL_CreateWindow("traa SDL Demo", theme::default_window_w,
                             theme::default_window_h, SDL_WINDOW_RESIZABLE);
  if (!window_) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return false;
  }

  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!renderer_) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window_);
    SDL_Quit();
    return false;
  }

  // initialize traa
  traa_config config;
  config.event_handler.on_error = on_traa_error;
  config.userdata = this;
  int ret = traa_init(&config);
  if (ret != TRAA_ERROR_NONE) {
    fprintf(stderr, "traa_init failed with error %d\n", ret);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
    return false;
  }

  register_panels();

  running_ = true;
  return true;
}

// ---------------------------------------------------------------------------
// run — main loop
// ---------------------------------------------------------------------------

void app::run() {
  while (running_) {
    // process events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running_ = false;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        mouse_x_ = event.motion.x;
        mouse_y_ = event.motion.y;
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
          mouse_clicked_ = true;
        }
        break;
      case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE) {
          running_ = false;
        }
        // digit keys 1-6 switch panels
        if (event.key.key >= SDLK_1 && event.key.key <= SDLK_6) {
          int index = static_cast<int>(event.key.key - SDLK_1);
          if (index < panel_mgr_.panel_count()) {
            panel_mgr_.switch_to(index);
          }
        }
        break;
      default:
        break;
      }
      panel_mgr_.handle_event(event);
    }

    panel_mgr_.update();

    // get window size and calculate layout
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);

    SDL_FRect sidebar_rect = ui::calc_sidebar_rect(w, h);
    SDL_FRect preview_rect = ui::calc_preview_rect(w, h);
    SDL_FRect status_rect = ui::calc_status_bar_rect(w, h);

    // clear screen
    SDL_SetRenderDrawColor(renderer_, theme::bg_dark.r, theme::bg_dark.g,
                           theme::bg_dark.b, theme::bg_dark.a);
    SDL_RenderClear(renderer_);

    // render UI regions
    render_sidebar(sidebar_rect);
    panel_mgr_.render(renderer_, preview_rect);
    render_status_bar(status_rect);

    SDL_RenderPresent(renderer_);
    SDL_Delay(16); // ~60 FPS

    // reset per-frame input state
    mouse_clicked_ = false;
  }
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------

void app::shutdown() {
  traa_release();

  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  SDL_Quit();
}

// ---------------------------------------------------------------------------
// register_panels — will be filled in later tasks when panels are created
// ---------------------------------------------------------------------------

void app::register_panels() {
  panel_mgr_.register_panel(std::make_unique<device_panel>());
  panel_mgr_.register_panel(std::make_unique<screen_source_panel>());
  panel_mgr_.register_panel(std::make_unique<snapshot_panel>());
  panel_mgr_.register_panel(std::make_unique<camera_panel>());
  panel_mgr_.register_panel(std::make_unique<screen_capture_panel>());
  panel_mgr_.register_panel(std::make_unique<log_panel>());
  panel_mgr_.set_app(this);
}

// ---------------------------------------------------------------------------
// render_sidebar
// ---------------------------------------------------------------------------

void app::render_sidebar(const SDL_FRect &area) {
  // fill sidebar background with a slightly lighter color than bg_dark
  SDL_SetRenderDrawColor(renderer_, theme::button.r, theme::button.g,
                         theme::button.b, theme::button.a);
  SDL_RenderFillRect(renderer_, &area);

  // draw panel navigation buttons
  constexpr float button_height = 36.0f;
  constexpr float button_margin = 4.0f;
  constexpr float button_padding_x = 8.0f;
  float y_offset = button_margin;

  int count = panel_mgr_.panel_count();
  for (int i = 0; i < count; ++i) {
    SDL_FRect btn_rect;
    btn_rect.x = area.x + button_padding_x;
    btn_rect.y = area.y + y_offset;
    btn_rect.w = area.w - button_padding_x * 2.0f;
    btn_rect.h = button_height;

    bool hovered = ui::hit_test(btn_rect, mouse_x_, mouse_y_);
    bool active = (i == panel_mgr_.active_index());

    ui::draw_button(renderer_, btn_rect, panel_mgr_.panel_name(i), hovered,
                    active);

    // handle click
    if (mouse_clicked_ && hovered) {
      panel_mgr_.switch_to(i);
    }

    y_offset += button_height + button_margin;
  }

  // draw separator line on the right edge
  SDL_SetRenderDrawColor(renderer_, theme::separator.r, theme::separator.g,
                         theme::separator.b, theme::separator.a);
  float sep_x = area.x + area.w - 1.0f;
  SDL_RenderLine(renderer_, sep_x, area.y, sep_x, area.y + area.h);
}

// ---------------------------------------------------------------------------
// render_status_bar
// ---------------------------------------------------------------------------

void app::render_status_bar(const SDL_FRect &area) {
  // fill status bar background
  SDL_SetRenderDrawColor(renderer_, theme::separator.r, theme::separator.g,
                         theme::separator.b, theme::separator.a);
  SDL_RenderFillRect(renderer_, &area);

  // draw separator line on top edge
  SDL_SetRenderDrawColor(renderer_, theme::highlight.r, theme::highlight.g,
                         theme::highlight.b, 0x40);
  SDL_RenderLine(renderer_, area.x, area.y, area.x + area.w, area.y);

  // draw status text on the left
  float text_x = area.x + 8.0f;
  float text_y =
      area.y +
      (area.h - static_cast<float>(SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) * 0.5f;
  ui::draw_text(renderer_, text_x, text_y, status_text_, theme::text_light);
}

// ---------------------------------------------------------------------------
// shared state accessors
// ---------------------------------------------------------------------------

int64_t app::selected_source_id() const { return selected_source_id_; }

void app::set_selected_source_id(int64_t id) { selected_source_id_ = id; }

const char *app::selected_source_title() const {
  return selected_source_title_;
}

void app::set_selected_source_title(const char *title) {
  if (title) {
    snprintf(selected_source_title_, sizeof(selected_source_title_), "%s",
             title);
  } else {
    selected_source_title_[0] = '\0';
  }
}

void app::set_status(const char *text) {
  if (text) {
    snprintf(status_text_, sizeof(status_text_), "%s", text);
  } else {
    status_text_[0] = '\0';
  }
}

void app::append_log(const char *message) {
  if (!message) {
    return;
  }
  std::lock_guard<std::mutex> lock(log_mutex_);
  log_buffer_.emplace_back(message);
  while (static_cast<int>(log_buffer_.size()) > k_max_log_entries) {
    log_buffer_.pop_front();
  }
}

SDL_Renderer *app::renderer() const { return renderer_; }

panel_manager &app::panels() { return panel_mgr_; }

const std::deque<std::string> &app::log_buffer() const {
  return log_buffer_;
}

std::mutex &app::log_mutex() { return log_mutex_; }

// ---------------------------------------------------------------------------
// traa error callback
// ---------------------------------------------------------------------------

void app::on_traa_error(const traa_userdata userdata, traa_error error,
                        const char *message) {
  auto *self = static_cast<app *>(userdata);
  if (!self) {
    return;
  }

  char buf[512];
  snprintf(buf, sizeof(buf), "traa error %d: %s", static_cast<int>(error),
           message ? message : "(null)");
  self->set_status(buf);
  self->append_log(buf);
}
