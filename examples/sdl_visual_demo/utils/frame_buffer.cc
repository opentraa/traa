#include "utils/frame_buffer.h"
#include "utils/frame_converter.h"

#include <algorithm>
#include <cstring>

void frame_buffer::push(const traa_video_frame *frame) {
  if (!frame || !frame->data || frame->width <= 0 || frame->height <= 0) {
    return;
  }

  int w = frame->width;
  int h = frame->height;

  // Calculate data size based on frame format
  size_t data_size = 0;
  switch (frame->format) {
  case TRAA_VIDEO_FRAME_FORMAT_BGRA:
    data_size = static_cast<size_t>(w) * h * 4;
    break;
  case TRAA_VIDEO_FRAME_FORMAT_I420:
    data_size = static_cast<size_t>(w) * h * 3 / 2;
    break;
  default:
    // Fall back to data_length for unknown formats
    if (frame->data_length > 0) {
      data_size = static_cast<size_t>(frame->data_length);
    } else {
      return;
    }
    break;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  buffer_.resize(data_size);
  std::memcpy(buffer_.data(), frame->data, data_size);
  width_ = w;
  height_ = h;
  format_ = frame->format;
  has_new_frame_ = true;
}

bool frame_buffer::pop_to_texture(SDL_Renderer *renderer, SDL_Texture **texture,
                                  int *width, int *height) {
  if (!renderer || !texture) {
    return false;
  }

  // Copy data under lock, then release lock before texture creation
  std::vector<uint8_t> local_buffer;
  int w = 0;
  int h = 0;
  traa_video_frame_format fmt = TRAA_VIDEO_FRAME_FORMAT_UNKNOWN;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_new_frame_) {
      return false;
    }
    local_buffer = buffer_;
    w = width_;
    h = height_;
    fmt = format_;
    has_new_frame_ = false;
  }

  SDL_Texture *new_texture = nullptr;

  switch (fmt) {
  case TRAA_VIDEO_FRAME_FORMAT_BGRA:
    // BGRA data can be uploaded directly to an SDL texture
    new_texture = frame_converter::bgra_to_texture(renderer, local_buffer.data(), w, h);
    break;
  case TRAA_VIDEO_FRAME_FORMAT_I420: {
    // I420 needs conversion via frame_converter
    traa_video_frame temp_frame;
    temp_frame.data = local_buffer.data();
    temp_frame.data_length = static_cast<int32_t>(local_buffer.size());
    temp_frame.width = w;
    temp_frame.height = h;
    temp_frame.format = TRAA_VIDEO_FRAME_FORMAT_I420;
    temp_frame.timestamp_ms = 0;
    new_texture = frame_converter::i420_to_texture(renderer, &temp_frame);
    break;
  }
  default:
    return false;
  }

  if (!new_texture) {
    return false;
  }

  *texture = new_texture;
  if (width) {
    *width = w;
  }
  if (height) {
    *height = h;
  }
  return true;
}

bool frame_buffer::has_new_frame() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return has_new_frame_;
}

int frame_buffer::buffer_width() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return width_;
}

int frame_buffer::buffer_height() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return height_;
}
