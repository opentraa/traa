#ifndef TRAA_SDL_DEMO_UTILS_FRAME_CONVERTER_H_
#define TRAA_SDL_DEMO_UTILS_FRAME_CONVERTER_H_

#include <SDL3/SDL.h>
#include <traa/base.h>
#include <cstdint>

class frame_converter {
public:
  // BGRA raw pixel data → SDL_Texture (SDL_PIXELFORMAT_BGRA32)
  // Returns nullptr on invalid input (nullptr data, width <= 0, height <= 0)
  // Caller owns the returned texture and must call SDL_DestroyTexture
  static SDL_Texture *bgra_to_texture(SDL_Renderer *renderer,
                                       const uint8_t *data,
                                       int width, int height);

  // I420 traa_video_frame → SDL_Texture
  // Internally converts I420 to BGRA, then creates texture
  // Returns nullptr on invalid input
  // Caller owns the returned texture
  static SDL_Texture *i420_to_texture(SDL_Renderer *renderer,
                                       const traa_video_frame *frame);
};

#endif // TRAA_SDL_DEMO_UTILS_FRAME_CONVERTER_H_
