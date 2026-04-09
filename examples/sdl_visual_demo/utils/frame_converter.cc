#include "utils/frame_converter.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

SDL_Texture *frame_converter::bgra_to_texture(SDL_Renderer *renderer,
                                               const uint8_t *data,
                                               int width, int height) {
  if (!renderer || !data || width <= 0 || height <= 0) {
    return nullptr;
  }

  SDL_Texture *texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STATIC, width, height);
  if (!texture) {
    return nullptr;
  }

  if (!SDL_UpdateTexture(texture, nullptr, data, width * 4)) {
    SDL_DestroyTexture(texture);
    return nullptr;
  }

  return texture;
}

// clamp a float value to [0, 255] and convert to uint8_t
static inline uint8_t clamp_u8(float v) {
  int i = static_cast<int>(v + 0.5f);
  return static_cast<uint8_t>(std::max(0, std::min(255, i)));
}

SDL_Texture *frame_converter::i420_to_texture(SDL_Renderer *renderer,
                                               const traa_video_frame *frame) {
  if (!renderer || !frame || !frame->data || frame->width <= 0 || frame->height <= 0) {
    return nullptr;
  }

  int w = frame->width;
  int h = frame->height;
  int half_w = w / 2;
  int half_h = h / 2;

  // I420 layout: Y plane, then U plane, then V plane
  const uint8_t *y_plane = frame->data;
  const uint8_t *u_plane = y_plane + w * h;
  const uint8_t *v_plane = u_plane + half_w * half_h;

  // allocate BGRA buffer
  std::vector<uint8_t> bgra(static_cast<size_t>(w) * h * 4);

  // convert YUV to BGRA using BT.601 coefficients
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w; ++col) {
      int y_index = row * w + col;
      int uv_index = (row / 2) * half_w + (col / 2);

      float y = static_cast<float>(y_plane[y_index]);
      float u = static_cast<float>(u_plane[uv_index]) - 128.0f;
      float v = static_cast<float>(v_plane[uv_index]) - 128.0f;

      // BT.601 conversion
      float r = y + 1.402f * v;
      float g = y - 0.344f * u - 0.714f * v;
      float b = y + 1.772f * u;

      size_t pixel = static_cast<size_t>(y_index) * 4;
      bgra[pixel + 0] = clamp_u8(b); // B
      bgra[pixel + 1] = clamp_u8(g); // G
      bgra[pixel + 2] = clamp_u8(r); // R
      bgra[pixel + 3] = 0xFF;        // A
    }
  }

  return bgra_to_texture(renderer, bgra.data(), w, h);
}
