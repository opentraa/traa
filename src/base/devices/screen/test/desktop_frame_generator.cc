/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/test/desktop_frame_generator.h"

#include "base/devices/screen/rgba_color.h"
#include "base/utils/time_utils.h"
#include "base/random.h"

#include <memory>
#include <stdint.h>
#include <string.h>

namespace traa {
namespace base {

inline namespace {

// Sets `updated_region` to `frame`. If `enlarge_updated_region` is
// true, this function will randomly enlarge each desktop_rect in
// `updated_region`. But the enlarged desktop_region won't excceed the
// frame->size(). If `add_random_updated_region` is true, several random
// rectangles will also be included in `frame`.
void set_updated_region(desktop_frame *frame, const desktop_region &updated_region,
                        bool enlarge_updated_region, int enlarge_range,
                        bool add_random_updated_region) {
  const desktop_rect screen_rect = desktop_rect::make_size(frame->size());
  random rd(time_micros());
  frame->mutable_updated_region()->clear();
  for (desktop_region::iterator it(updated_region); !it.is_at_end(); it.advance()) {
    desktop_rect rect = it.rect();
    if (enlarge_updated_region && enlarge_range > 0) {
      rect.extend(rd.rand(enlarge_range), rd.rand(enlarge_range), rd.rand(enlarge_range),
                  rd.rand(enlarge_range));
      rect.intersect_with(screen_rect);
    }
    frame->mutable_updated_region()->add_rect(rect);
  }

  if (add_random_updated_region) {
    for (int i = rd.rand(10); i >= 0; i--) {
      // At least a 1 x 1 updated region.
      const int left = rd.rand(0, frame->size().width() - 2);
      const int top = rd.rand(0, frame->size().height() - 2);
      const int right = rd.rand(left + 1, frame->size().width());
      const int bottom = rd.rand(top + 1, frame->size().height());
      frame->mutable_updated_region()->add_rect(desktop_rect::make_ltrb(left, top, right, bottom));
    }
  }
}

// Paints pixels in `rect` of `frame` to `color`.
void paint_rect(desktop_frame *frame, desktop_rect rect, rgba_color rgba_color) {
  static_assert(desktop_frame::k_bytes_per_pixel == sizeof(uint32_t),
                "k_bytes_per_pixel should be 4.");
  uint32_t color = rgba_color.to_uint32();
  uint8_t *row = frame->get_frame_data_at_pos(rect.top_left());
  for (int i = 0; i < rect.height(); i++) {
    uint32_t *column = reinterpret_cast<uint32_t *>(row);
    for (int j = 0; j < rect.width(); j++) {
      column[j] = color;
    }
    row += frame->stride();
  }
}

// Paints pixels in `region` of `frame` to `color`.
void paint_region(desktop_frame *frame, desktop_region *region, rgba_color rgba_color) {
  region->intersect_with(desktop_rect::make_size(frame->size()));
  for (desktop_region::iterator it(*region); !it.is_at_end(); it.advance()) {
    paint_rect(frame, it.rect(), rgba_color);
  }
}

} // namespace

desktop_frame_generator::desktop_frame_generator() {}
desktop_frame_generator::~desktop_frame_generator() {}

desktop_frame_painter::desktop_frame_painter() {}
desktop_frame_painter::~desktop_frame_painter() {}

painter_desktop_frame_generator::painter_desktop_frame_generator()
    : size_(1024, 768), return_frame_(true), provide_updated_region_hints_(false),
      enlarge_updated_region_(false), enlarge_range_(20), add_random_updated_region_(false),
      painter_(nullptr) {}
painter_desktop_frame_generator::~painter_desktop_frame_generator() {}

std::unique_ptr<desktop_frame>
painter_desktop_frame_generator::get_next_frame(shared_memory_factory *factory) {
  if (!return_frame_) {
    return nullptr;
  }

  std::unique_ptr<desktop_frame> frame = std::unique_ptr<desktop_frame>(
      factory ? shared_memory_desktop_frame::create(size_, factory).release()
              : new basic_desktop_frame(size_));
  if (painter_) {
    desktop_region updated_region;
    if (!painter_->paint(frame.get(), &updated_region)) {
      return nullptr;
    }

    if (provide_updated_region_hints_) {
      set_updated_region(frame.get(), updated_region, enlarge_updated_region_, enlarge_range_,
                         add_random_updated_region_);
    } else {
      frame->mutable_updated_region()->set_rect(desktop_rect::make_size(frame->size()));
    }
  }

  return frame;
}

desktop_size *painter_desktop_frame_generator::size() { return &size_; }

void painter_desktop_frame_generator::set_return_frame(bool return_frame) {
  return_frame_ = return_frame;
}

void painter_desktop_frame_generator::set_provide_updated_region_hints(
    bool provide_updated_region_hints) {
  provide_updated_region_hints_ = provide_updated_region_hints;
}

void painter_desktop_frame_generator::set_enlarge_updated_region(bool enlarge_updated_region) {
  enlarge_updated_region_ = enlarge_updated_region;
}

void painter_desktop_frame_generator::set_enlarge_range(int enlarge_range) {
  enlarge_range_ = enlarge_range;
}

void painter_desktop_frame_generator::set_add_random_updated_region(
    bool add_random_updated_region) {
  add_random_updated_region_ = add_random_updated_region;
}

void painter_desktop_frame_generator::set_desktop_frame_painter(desktop_frame_painter *painter) {
  painter_ = painter;
}

black_white_desktop_frame_painter::black_white_desktop_frame_painter() {}
black_white_desktop_frame_painter::~black_white_desktop_frame_painter() {}

desktop_region *black_white_desktop_frame_painter::updated_region() { return &updated_region_; }

bool black_white_desktop_frame_painter::paint(desktop_frame *frame,
                                              desktop_region *updated_region) {
  memset(frame->data(), 0, frame->stride() * frame->size().height());
  paint_region(frame, &updated_region_, rgba_color(0xFFFFFFFF));
  updated_region_.swap(updated_region);
  return true;
}

} // namespace base
} // namespace traa
