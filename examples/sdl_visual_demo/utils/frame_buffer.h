#ifndef TRAA_SDL_DEMO_UTILS_FRAME_BUFFER_H_
#define TRAA_SDL_DEMO_UTILS_FRAME_BUFFER_H_

#include <SDL3/SDL.h>
#include <traa/base.h>
#include <cstdint>
#include <mutex>
#include <vector>

class frame_buffer {
public:
  // Called on capture thread: copy frame data into internal buffer
  void push(const traa_video_frame *frame);

  // Called on render thread: get latest frame as SDL_Texture
  // Returns true if a new frame was available, texture is updated
  // Caller owns the returned texture and must call SDL_DestroyTexture
  bool pop_to_texture(SDL_Renderer *renderer, SDL_Texture **texture,
                      int *width, int *height);

  // For testing: check if there's a new frame without consuming it
  bool has_new_frame() const;

  // For testing: get buffer dimensions
  int buffer_width() const;
  int buffer_height() const;

private:
  mutable std::mutex mutex_;
  std::vector<uint8_t> buffer_;
  int width_ = 0;
  int height_ = 0;
  traa_video_frame_format format_ = TRAA_VIDEO_FRAME_FORMAT_UNKNOWN;
  bool has_new_frame_ = false;
};

#endif // TRAA_SDL_DEMO_UTILS_FRAME_BUFFER_H_
