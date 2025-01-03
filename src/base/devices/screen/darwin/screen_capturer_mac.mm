/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/screen_capturer_mac.h"

#include "base/checks.h"
#include "base/devices/screen/darwin/desktop_frame_provider.h"
#include "base/devices/screen/darwin/window_list_utils.h"
#include "base/logger.h"
#include "base/sdk/objc/helpers/scoped_cftyperef.h"
#include "base/utils/time_utils.h"

#include <utility>

namespace traa {
namespace base {

namespace {

// Scales all coordinates of a rect by a specified factor.
desktop_rect scale_and_round_cgrect(const CGRect &rect, float scale) {
  return desktop_rect::make_ltrb(
      static_cast<int>(std::floor(rect.origin.x * scale)),
      static_cast<int>(std::floor(rect.origin.y * scale)),
      static_cast<int>(std::ceil((rect.origin.x + rect.size.width) * scale)),
      static_cast<int>(std::ceil((rect.origin.y + rect.size.height) * scale)));
}

// Copy pixels in the `rect` from `src_place` to `dest_plane`. `rect` should be
// relative to the origin of `src_plane` and `dest_plane`.
void copy_rect(const uint8_t *src_plane, int src_plane_stride, uint8_t *dest_plane,
               int dest_plane_stride, int bytes_per_pixel, const desktop_rect &rect) {
  // Get the address of the starting point.
  const int src_y_offset = src_plane_stride * rect.top();
  const int dest_y_offset = dest_plane_stride * rect.top();
  const int x_offset = bytes_per_pixel * rect.left();
  src_plane += src_y_offset + x_offset;
  dest_plane += dest_y_offset + x_offset;

  // Copy pixels in the rectangle line by line.
  const int bytes_per_line = bytes_per_pixel * rect.width();
  const int height = rect.height();
  for (int i = 0; i < height; ++i) {
    memcpy(dest_plane, src_plane, bytes_per_line);
    src_plane += src_plane_stride;
    dest_plane += dest_plane_stride;
  }
}

// Returns an array of CGWindowID for all the on-screen windows except
// `window_to_exclude`, or NULL if the window is not found or it fails. The
// caller should release the returned CFArrayRef.
CFArrayRef create_window_list_with_exclusion(CGWindowID window_to_exclude) {
  if (!window_to_exclude)
    return nullptr;

  CFArrayRef all_windows =
      CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
  if (!all_windows)
    return nullptr;

  CFMutableArrayRef returned_array =
      CFArrayCreateMutable(nullptr, CFArrayGetCount(all_windows), nullptr);

  bool found = false;
  for (CFIndex i = 0; i < CFArrayGetCount(all_windows); ++i) {
    CFDictionaryRef window =
        reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(all_windows, i));

    CGWindowID id = static_cast<CGWindowID>(get_window_id(window));
    if (id == window_to_exclude) {
      found = true;
      continue;
    }
    CFArrayAppendValue(returned_array, reinterpret_cast<void *>(id));
  }
  CFRelease(all_windows);

  if (!found) {
    CFRelease(returned_array);
    returned_array = nullptr;
  }
  return returned_array;
}

// Returns the bounds of `window` in physical pixels, enlarged by a small amount
// on four edges to take account of the border/shadow effects.
desktop_rect get_excluded_window_pixel_bounds(CGWindowID window, float dip_to_pixel_scale) {
  // The amount of pixels to add to the actual window bounds to take into
  // account of the border/shadow effects.
  static const int k_border_effect_size = 20;
  CGRect rect;
  CGWindowID ids[1];
  ids[0] = window;

  CFArrayRef window_id_array =
      CFArrayCreate(nullptr, reinterpret_cast<const void **>(&ids), 1, nullptr);
  CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);

  if (CFArrayGetCount(window_array) > 0) {
    CFDictionaryRef win =
        reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, 0));
    CFDictionaryRef bounds_ref =
        reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(win, kCGWindowBounds));
    CGRectMakeWithDictionaryRepresentation(bounds_ref, &rect);
  }

  CFRelease(window_id_array);
  CFRelease(window_array);

  rect.origin.x -= k_border_effect_size;
  rect.origin.y -= k_border_effect_size;
  rect.size.width += k_border_effect_size * 2;
  rect.size.height += k_border_effect_size * 2;
  // `rect` is in DIP, so convert to physical pixels.
  return scale_and_round_cgrect(rect, dip_to_pixel_scale);
}

// Create an image of the given region using the given `window_list`.
// `pixel_bounds` should be in the primary display's coordinate in physical
// pixels.
scoped_cf_type_ref<CGImageRef> create_excluded_window_region_image(const desktop_rect &pixel_bounds,
                                                                   float dip_to_pixel_scale,
                                                                   CFArrayRef window_list) {
  CGRect window_bounds;
  // The origin is in DIP while the size is in physical pixels. That's what
  // CGWindowListCreateImageFromArray expects.
  window_bounds.origin.x = pixel_bounds.left() / dip_to_pixel_scale;
  window_bounds.origin.y = pixel_bounds.top() / dip_to_pixel_scale;
  window_bounds.size.width = pixel_bounds.width();
  window_bounds.size.height = pixel_bounds.height();

  return scoped_cf_type_ref<CGImageRef>(
      CGWindowListCreateImageFromArray(window_bounds, window_list, kCGWindowImageDefault));
}

} // namespace

screen_capturer_mac::screen_capturer_mac(
    std::shared_ptr<desktop_configuration_monitor> desktop_config_monitor,
    bool detect_updated_region, bool allow_iosurface)
    : detect_updated_region_(detect_updated_region),
      desktop_config_monitor_(desktop_config_monitor), desktop_frame_provider_(allow_iosurface) {
  LOG_INFO("allow_iosurface: {}", allow_iosurface);
}

screen_capturer_mac::~screen_capturer_mac() {
  release_buffers();
  unregister_refresh_and_move_handlers();
}

bool screen_capturer_mac::init() {
  LOG_INFO("screen_capturer_mac::init");
  desktop_config_ = desktop_config_monitor_->get_desktop_configuration();
  return true;
}

void screen_capturer_mac::release_buffers() {
  // The buffers might be in use by the encoder, so don't delete them here.
  // Instead, mark them as "needs update"; next time the buffers are used by
  // the capturer, they will be recreated if necessary.
  queue_.reset();
}

void screen_capturer_mac::start(capture_callback *callback) {
  TRAA_DCHECK(!callback_);
  TRAA_DCHECK(callback);
  LOG_INFO("screen_capturer_mac::start, target display id {}", current_display_);

  callback_ = callback;
  // Start and operate CGDisplayStream handler all from capture thread.
  if (!register_refresh_and_move_handlers()) {
    LOG_ERROR("failed to register refresh and move handlers.");
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent, nullptr);
    return;
  }
  screen_configuration_changed();
}

void screen_capturer_mac::capture_frame() {
  LOG_INFO("screen_capturer_mac::capture_frame");
  int64_t capture_start_time_nanos = time_nanos();

  queue_.move_to_next_frame();
  if (queue_.current_frame() && queue_.current_frame()->is_shared()) {
    LOG_WARN("overwriting frame that is still shared.");
  }

  desktop_configuration new_config = desktop_config_monitor_->get_desktop_configuration();
  if (!desktop_config_.equals(new_config)) {
    desktop_config_ = new_config;
    // If the display configuraiton has changed then refresh capturer data
    // structures. Occasionally, the refresh and move handlers are lost when
    // the screen mode changes, so re-register them here.
    unregister_refresh_and_move_handlers();
    if (!register_refresh_and_move_handlers()) {
      LOG_ERROR("failed to register refresh and move handlers.");
      callback_->on_capture_result(desktop_capturer::capture_result::error_permanent, nullptr);
      return;
    }
    screen_configuration_changed();
  }

  // When screen is zoomed in/out, OSX only updates the part of Rects currently
  // displayed on screen, with relative location to current top-left on screen.
  // This will cause problems when we copy the dirty regions to the captured
  // image. So we invalidate the whole screen to copy all the screen contents.
  // With CGI method, the zooming will be ignored and the whole screen contents
  // will be captured as before.
  // With IOSurface method, the zoomed screen contents will be captured.
  if (UAZoomEnabled()) {
    helper_.invalidate_screen(screen_pixel_bounds_.size());
  }

  desktop_region region;
  helper_.take_invalid_region(&region);

  // If the current buffer is from an older generation then allocate a new one.
  // Note that we can't reallocate other buffers at this point, since the caller
  // may still be reading from them.
  if (!queue_.current_frame())
    queue_.replace_current_frame(shared_desktop_frame::wrap(create_frame()));

  desktop_frame *current_frame = queue_.current_frame();

  if (!cg_blit(*current_frame, region)) {
    callback_->on_capture_result(desktop_capturer::capture_result::error_permanent, nullptr);
    return;
  }
  std::unique_ptr<desktop_frame> new_frame = queue_.current_frame()->share();
  if (detect_updated_region_) {
    *new_frame->mutable_updated_region() = region;
  } else {
    new_frame->mutable_updated_region()->add_rect(desktop_rect::make_size(new_frame->size()));
  }

  if (current_display_) {
    const display_configuration *config = desktop_config_.find_by_id(current_display_);
    if (config) {
      new_frame->set_top_left(
          config->bounds.top_left().subtract(desktop_config_.bounds.top_left()));
    }
  }

  helper_.set_size_most_recent(new_frame->size());

  new_frame->set_capture_time_ms((time_nanos() - capture_start_time_nanos) /
                                 k_num_nanosecs_per_millisec);
  callback_->on_capture_result(desktop_capturer::capture_result::success, std::move(new_frame));
}

void screen_capturer_mac::set_excluded_window(win_id_t window) {
  excluded_window_ = static_cast<CGWindowID>(window);
}

bool screen_capturer_mac::get_source_list(source_list_t *screens) {
  TRAA_DCHECK(screens->size() == 0);

  for (display_configuration_array_t::iterator it = desktop_config_.displays.begin();
       it != desktop_config_.displays.end(); ++it) {
    screens->push_back({it->id, std::string()});
  }
  return true;
}

bool screen_capturer_mac::select_source(source_id_t id) {
  if (id == k_screen_id_full) {
    current_display_ = 0;
  } else {
    const display_configuration *config =
        desktop_config_.find_by_id(static_cast<CGDirectDisplayID>(id));
    if (!config)
      return false;
    current_display_ = config->id;
  }

  screen_configuration_changed();
  return true;
}

bool screen_capturer_mac::cg_blit(const desktop_frame &frame, const desktop_region &region) {
  // If not all screen region is dirty, copy the entire contents of the previous capture buffer,
  // to capture over.
  if (queue_.previous_frame() && !region.equals(desktop_region(screen_pixel_bounds_))) {
    memcpy(frame.data(), queue_.previous_frame()->data(), frame.stride() * frame.size().height());
  }

  display_configuration_array_t displays_to_capture;
  if (current_display_) {
    // Capturing a single screen. Note that the screen id may change when
    // screens are added or removed.
    const display_configuration *config = desktop_config_.find_by_id(current_display_);
    if (config) {
      displays_to_capture.push_back(*config);
    } else {
      LOG_ERROR("the selected screen cannot be found for capturing.");
      return false;
    }
  } else {
    // Capturing the whole desktop.
    displays_to_capture = desktop_config_.displays;
  }

  // Create the window list once for all displays.
  CFArrayRef window_list = create_window_list_with_exclusion(excluded_window_);

  for (size_t i = 0; i < displays_to_capture.size(); ++i) {
    const display_configuration &display_config = displays_to_capture[i];

    // Capturing mixed-DPI on one surface is hard, so we only return displays
    // that match the "primary" display's DPI. The primary display is always
    // the first in the list.
    if (i > 0 && display_config.dip_to_pixel_scale != displays_to_capture[0].dip_to_pixel_scale) {
      continue;
    }
    // Determine the display's position relative to the desktop, in pixels.
    desktop_rect display_bounds = display_config.pixel_bounds;
    display_bounds.translate(-screen_pixel_bounds_.left(), -screen_pixel_bounds_.top());

    // Determine which parts of the blit region, if any, lay within the monitor.
    desktop_region copy_region = region;
    copy_region.intersect_with(display_bounds);
    if (copy_region.is_empty())
      continue;

    // translate the region to be copied into display-relative coordinates.
    copy_region.translate(-display_bounds.left(), -display_bounds.top());

    desktop_rect excluded_window_bounds;
    scoped_cf_type_ref<CGImageRef> excluded_image;
    if (excluded_window_ && window_list) {
      // Get the region of the excluded window relative the primary display.
      excluded_window_bounds =
          get_excluded_window_pixel_bounds(excluded_window_, display_config.dip_to_pixel_scale);
      excluded_window_bounds.intersect_with(display_config.pixel_bounds);

      // Create the image under the excluded window first, because it's faster
      // than captuing the whole display.
      if (!excluded_window_bounds.is_empty()) {
        excluded_image = create_excluded_window_region_image(
            excluded_window_bounds, display_config.dip_to_pixel_scale, window_list);
      }
    }

    std::unique_ptr<desktop_frame> frame_source =
        desktop_frame_provider_.take_latest_frame_for_display(display_config.id);
    if (!frame_source) {
      continue;
    }

    const uint8_t *display_base_address = frame_source->data();
    int src_bytes_per_row = frame_source->stride();
    TRAA_DCHECK(display_base_address);

    // `frame_source` size may be different from display_bounds in case the screen was
    // resized recently.
    copy_region.intersect_with(frame_source->rect());

    // Copy the dirty region from the display buffer into our desktop buffer.
    uint8_t *out_ptr = frame.get_frame_data_at_pos(display_bounds.top_left());
    for (desktop_region::iterator it(copy_region); !it.is_at_end(); it.advance()) {
      copy_rect(display_base_address, src_bytes_per_row, out_ptr, frame.stride(),
                desktop_frame::k_bytes_per_pixel, it.rect());
    }

    if (excluded_image) {
      CGDataProviderRef provider = CGImageGetDataProvider(excluded_image.get());
      scoped_cf_type_ref<CFDataRef> excluded_image_data(CGDataProviderCopyData(provider));
      TRAA_DCHECK(excluded_image_data);
      display_base_address = CFDataGetBytePtr(excluded_image_data.get());
      src_bytes_per_row = static_cast<int>(CGImageGetBytesPerRow(excluded_image.get()));

      // translate the bounds relative to the desktop, because `frame` data
      // starts from the desktop top-left corner.
      desktop_rect window_bounds_relative_to_desktop(excluded_window_bounds);
      window_bounds_relative_to_desktop.translate(-screen_pixel_bounds_.left(),
                                                  -screen_pixel_bounds_.top());

      desktop_rect rect_to_copy = desktop_rect::make_size(excluded_window_bounds.size());
      rect_to_copy.intersect_with(
          desktop_rect::make_wh(static_cast<int>(CGImageGetWidth(excluded_image.get())),
                                static_cast<int>(CGImageGetHeight(excluded_image.get()))));

      if (CGImageGetBitsPerPixel(excluded_image.get()) / 8 == desktop_frame::k_bytes_per_pixel) {
        copy_rect(display_base_address, src_bytes_per_row,
                  frame.get_frame_data_at_pos(window_bounds_relative_to_desktop.top_left()),
                  frame.stride(), desktop_frame::k_bytes_per_pixel, rect_to_copy);
      }
    }
  }
  if (window_list)
    CFRelease(window_list);
  return true;
}

void screen_capturer_mac::screen_configuration_changed() {
  if (current_display_) {
    const display_configuration *config = desktop_config_.find_by_id(current_display_);
    screen_pixel_bounds_ = config ? config->pixel_bounds : desktop_rect();
    dip_to_pixel_scale_ = config ? config->dip_to_pixel_scale : 1.0f;
  } else {
    screen_pixel_bounds_ = desktop_config_.pixel_bounds;
    dip_to_pixel_scale_ = desktop_config_.dip_to_pixel_scale;
  }

  // Release existing buffers, which will be of the wrong size.
  release_buffers();

  // Clear the dirty region, in case the display is down-sizing.
  helper_.clear_invalid_region();

  // Re-mark the entire desktop as dirty.
  helper_.invalidate_screen(screen_pixel_bounds_.size());

  // Make sure the frame buffers will be reallocated.
  queue_.reset();
}

bool screen_capturer_mac::register_refresh_and_move_handlers() {
  if (!desktop_frame_provider_.allow_iosurface()) {
    return true;
  }

  desktop_config_ = desktop_config_monitor_->get_desktop_configuration();
  for (const auto &config : desktop_config_.displays) {
    size_t pixel_width = config.pixel_bounds.width();
    size_t pixel_height = config.pixel_bounds.height();
    if (pixel_width == 0 || pixel_height == 0)
      continue;
    CGDirectDisplayID display_id = config.id;
    desktop_vector display_origin = config.pixel_bounds.top_left();

    CGDisplayStreamFrameAvailableHandler handler =
        ^(CGDisplayStreamFrameStatus status, uint64_t /* display_time */,
          IOSurfaceRef frame_surface, CGDisplayStreamUpdateRef updateRef) {
          if (status == kCGDisplayStreamFrameStatusStopped)
            return;

          // Only pay attention to frame updates.
          if (status != kCGDisplayStreamFrameStatusFrameComplete)
            return;

          size_t count = 0;
          const CGRect *rects =
              CGDisplayStreamUpdateGetRects(updateRef, kCGDisplayStreamUpdateDirtyRects, &count);
          if (count != 0) {
            // According to CGDisplayStream.h, it's safe to call
            // CGDisplayStreamStop() from within the callback.
            screen_refresh(display_id, static_cast<CGRectCount>(count), rects, display_origin,
                           frame_surface);
          }
        };

    scoped_cf_type_ref<CFDictionaryRef> properties_dict(
        CFDictionaryCreate(kCFAllocatorDefault, (const void *[]){kCGDisplayStreamShowCursor},
                           (const void *[]){kCFBooleanFalse}, 1, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));

    CGDisplayStreamRef display_stream = CGDisplayStreamCreate(
        display_id, pixel_width, pixel_height, 'BGRA', properties_dict.get(), handler);

    if (display_stream) {
      CGError error = CGDisplayStreamStart(display_stream);
      if (error != kCGErrorSuccess)
        return false;

      CFRunLoopSourceRef source = CGDisplayStreamGetRunLoopSource(display_stream);
      CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
      display_streams_.push_back(display_stream);
    }
  }

  return true;
}

void screen_capturer_mac::unregister_refresh_and_move_handlers() {
  for (CGDisplayStreamRef stream : display_streams_) {
    CFRunLoopSourceRef source = CGDisplayStreamGetRunLoopSource(stream);
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
    CGDisplayStreamStop(stream);
    CFRelease(stream);
  }
  display_streams_.clear();

  // Release obsolete io surfaces.
  desktop_frame_provider_.release();
}

void screen_capturer_mac::screen_refresh(CGDirectDisplayID display_id, CGRectCount count,
                                         const CGRect *rect_array, desktop_vector display_origin,
                                         IOSurfaceRef io_surface) {
  if (screen_pixel_bounds_.is_empty())
    screen_configuration_changed();

  // The refresh rects are in display coordinates. We want to translate to
  // framebuffer coordinates. If a specific display is being captured, then no
  // change is necessary. If all displays are being captured, then we want to
  // translate by the origin of the display.
  desktop_vector translate_vector;
  if (!current_display_)
    translate_vector = display_origin;

  desktop_region region;
  for (CGRectCount i = 0; i < count; ++i) {
    // All rects are already in physical pixel coordinates.
    desktop_rect rect =
        desktop_rect::make_xywh(rect_array[i].origin.x, rect_array[i].origin.y,
                                rect_array[i].size.width, rect_array[i].size.height);

    rect.translate(translate_vector);

    region.add_rect(rect);
  }
  // Always having the latest iosurface before invalidating a region.
  // See https://bugs.chromium.org/p/webrtc/issues/detail?id=8652 for details.
  desktop_frame_provider_.invalidate_iosurface(
      display_id, scoped_cf_type_ref<IOSurfaceRef>(io_surface, retain_policy::retain));
  helper_.invalidate_region(region);
}

std::unique_ptr<desktop_frame> screen_capturer_mac::create_frame() {
  std::unique_ptr<desktop_frame> frame(new basic_desktop_frame(screen_pixel_bounds_.size()));
  frame->set_dpi(desktop_vector(desktop_frame::k_standard_dpi * dip_to_pixel_scale_,
                                desktop_frame::k_standard_dpi * dip_to_pixel_scale_));
  return frame;
}

} // namespace base
} // namespace traa
