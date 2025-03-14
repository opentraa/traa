/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/enumerator.h"

#include "base/devices/screen/darwin/desktop_configuration.h"
#include "base/devices/screen/darwin/window_list_utils.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/utils.h"
#include "base/folder/folder.h"
#include "base/logger.h"

#include <vector>

#include <functional>
#include <stdlib.h>
#include <string.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

namespace traa {
namespace base {

NSBitmapImageRep *get_image_rep(bool is_window, CGWindowID window_id) {
  CGImageRef image_ref = nil;
  if (is_window) {
    image_ref = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, window_id,
                                        kCGWindowImageBoundsIgnoreFraming |
                                            kCGWindowImageNominalResolution);
  } else {
    image_ref = CGDisplayCreateImage(window_id);
  }

  if (image_ref != nil) {
    NSBitmapImageRep *image_rep = [[NSBitmapImageRep alloc] initWithCGImage:image_ref];
    CGImageRelease(image_ref);
    return image_rep;
  }
  return nil;
}

NSBitmapImageRep *scale_image(NSBitmapImageRep *origin_image_rep, CGSize scaled_size) {
  if (origin_image_rep == nil || scaled_size.width <= 0 || scaled_size.height <= 0) {
    return nil;
  }

  size_t width = scaled_size.width;
  size_t height = scaled_size.height;

  CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  CGContextRef context = CGBitmapContextCreate(NULL, (size_t)width, (size_t)height,
                                               8, // PARAM(bitsPerComponent, 8),
                                               0, // PARAM(bytesPerRow,(size_t)(width * 4)),
                                               color_space, kCGImageAlphaPremultipliedLast);
  CGImageRef src_image = [origin_image_rep CGImage];
  if (src_image == nullptr || context == nullptr) {
    return nil;
  }

  CGRect rect = CGRectMake((CGFloat)0.f, (CGFloat)0.f, (CGFloat)width, (CGFloat)height);
  CGContextDrawImage(context, rect, src_image);
  CGImageRef dest_image = CGBitmapContextCreateImage(context);
  CGContextRelease(context);
  CGColorSpaceRelease(color_space);

  NSBitmapImageRep *scaled_image_rep = NULL;
  if (dest_image) {
    scaled_image_rep = [[NSBitmapImageRep alloc] initWithCGImage:dest_image];
    CGImageRelease(dest_image);
    return scaled_image_rep;
  }
  return nil;
}

bool is_source_id_valid(const int64_t source_id, bool &is_window) {
  if (source_id <= 0) {
    return false;
  }

  // try to find source_id in the screen list
  NSArray *screen_array = NSScreen.screens;
  for (size_t i = 0; i < screen_array.count; i++) {
    NSScreen *screen = screen_array[i];
    CGDirectDisplayID screen_id = static_cast<CGDirectDisplayID>(
        [[screen.deviceDescription objectForKey:@"NSScreenNumber"] unsignedIntValue]);
    if (source_id == static_cast<int64_t>(screen_id)) {
      is_window = false;
      return true;
    }
  }

  // try to find source_id in the window list
  CFArrayRef window_array = CGWindowListCopyWindowInfo(
      kCGWindowListOptionAll | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
  for (CFIndex i = 0; i < CFArrayGetCount(window_array); i++) {
    CFDictionaryRef window =
        reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, i));
    if (!window) {
      continue;
    }

    CFNumberRef window_id =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowNumber));
    if (!window_id) {
      continue;
    }

    int64_t window_id_value;
    if (!CFNumberGetValue(window_id, kCFNumberSInt64Type, &window_id_value) ||
        window_id_value <= 0) {
      continue;
    }

    if (source_id == window_id_value) {
      is_window = true;
      return true;
    }
  }

  return false;
}

void dump_image_to_file(NSBitmapImageRep *image_rep, const char *file_name) {
  if (image_rep == nil || file_name == nullptr) {
    return;
  }

  std::string file_path = folder::get_config_folder() + file_name;

  NSData *data = [image_rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
  if (data) {
    [data writeToFile:[NSString stringWithUTF8String:file_path.c_str()] atomically:YES];
  }
}

int enum_windows(const traa_size icon_size, const traa_size thumbnail_size,
                 const unsigned int external_flags, std::vector<traa_screen_source_info> &infos) {
  // Only get on screen, non-desktop windows.
  // According to
  // https://developer.apple.com/documentation/coregraphics/cgwindowlistoption/1454105-optiononscreenonly
  // , when kCGWindowListOptionOnScreenOnly is used, the order of windows are in
  // decreasing z-order.
  CFArrayRef window_array = CGWindowListCopyWindowInfo(
      kCGWindowListOptionAll | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
  if (!window_array)
    return false;

  desktop_configuration desktop_config =
      desktop_configuration::current(desktop_configuration::COORDINATE_TOP_LEFT);

  // Check windows to make sure they have an id, title, and use window layer
  // other than 0.
  CFIndex count = CFArrayGetCount(window_array);
  for (CFIndex i = 0; i < count; i++) {
    CFDictionaryRef window =
        reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, i));
    if (!window) {
      continue;
    }

    CFNumberRef window_id =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowNumber));
    if (!window_id) {
      continue;
    }

    traa_screen_source_info window_info;
    window_info.is_window = true;

    if (!CFNumberGetValue(window_id, kCFNumberSInt64Type, &window_info.id) || window_info.id <= 0) {
      continue;
    }

    // Skip windows with layer!=0 (menu, dock).
    CFNumberRef window_layer =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowLayer));
    if (!window_layer) {
      continue;
    }

    int layer;
    if (!CFNumberGetValue(window_layer, kCFNumberIntType, &layer)
        /* || layer != kCGNormalWindowLevel */
        || layer < kCGNormalWindowLevel || layer == kCGDockWindowLevel
        /*|| (!(external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_SKIP_ZERO_LAYER_WINDOWS) && layer !=
           0)*/
    ) {
      continue;
    }

    window_info.is_minimized = !is_window_on_screen(window);
    window_info.is_maximized = is_window_full_screen(desktop_config, window);

    // Skip windows that are minimized and not full screen.
    if ((external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_MINIMIZED) && window_info.is_minimized
        /* && !window_info.is_maximized */
    ) {
      continue;
    }

    // Skip windows that are not visible.
    CFNumberRef window_alpha =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowAlpha));
    if (!window_alpha) {
      continue;
    } else {
      int alpha;
      if (!CFNumberGetValue(window_alpha, kCFNumberIntType, &alpha) || alpha <= 0) {
        continue;
      }
    }

    // Skip windows that are not sharing state.
    CFNumberRef window_sharing_state =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowSharingState));
    if (!window_sharing_state) {
      continue;
    } else {
      int sharing_state;
      if (!CFNumberGetValue(window_sharing_state, kCFNumberIntType, &sharing_state) ||
          sharing_state != 1) {
        continue;
      }
    }

    // Skip windows that do not have pid.
    CFNumberRef window_pid =
        reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(window, kCGWindowOwnerPID));
    if (!window_pid) {
      continue;
    }

    std::string str_title = get_window_title(window);
    std::string str_owner_name = get_window_owner_name(window);
    if (str_title.empty() && str_owner_name.empty() &&
        !(external_flags & TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNTITLED)) {
      continue;
    }
    if (str_title.empty()) {
      str_title = std::move(str_owner_name);
    }
    if (!str_title.empty()) {
      strncpy(const_cast<char *>(window_info.title), str_title.c_str(),
              std::min(sizeof(window_info.title) - 1, str_title.size()));
    }

    CGRect window_bounds = CGRectZero;
    if (CFDictionaryContainsKey(window, kCGWindowBounds)) {
      CFDictionaryRef rect = (CFDictionaryRef)CFDictionaryGetValue(window, kCGWindowBounds);
      if (rect) {
        CGRectMakeWithDictionaryRepresentation(rect, &window_bounds);
      }
    }
    if (window_bounds.size.width <= 96 || window_bounds.size.height <= 96) {
      continue;
    }
    window_info.rect = desktop_rect::make_xywh(window_bounds.origin.x, window_bounds.origin.y,
                                               window_bounds.size.width, window_bounds.size.height)
                           .to_traa_rect();

    // get process path and icon
    NSRunningApplication *running_app = [NSRunningApplication
        runningApplicationWithProcessIdentifier:((__bridge NSNumber *)window_pid).intValue];
    if (running_app) {
      std::string process_path = [[running_app bundleURL] fileSystemRepresentation];
      if (!process_path.empty()) {
        strncpy(const_cast<char *>(window_info.process_path), process_path.c_str(),
                std::min(sizeof(window_info.process_path) - 1, process_path.size()));
      } else if (external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_NOPROCESS_PATH) {
        continue;
      }

      // only get icon for the window when icon_size is set.
      if (icon_size.width > 0 && icon_size.height > 0 && running_app.icon &&
          running_app.icon.representations.count > 0) {
        NSImage *icon = running_app.icon;
        NSBitmapImageRep *icon_rep =
            [[NSBitmapImageRep alloc] initWithData:[icon TIFFRepresentation]];
        if (icon_rep) {
          desktop_size icon_scaled_size =
              calc_scaled_size(desktop_size(icon_rep.size.width, icon_rep.size.height), icon_size);
          CGSize icon_scaled_ns_size =
              CGSizeMake(icon_scaled_size.width(), icon_scaled_size.height());
          NSBitmapImageRep *icon_scaled_image_rep = scale_image(icon_rep, icon_scaled_ns_size);
          if (icon_scaled_image_rep) {
            size_t icon_data_size = [icon_scaled_image_rep pixelsWide] *
                                    [icon_scaled_image_rep pixelsHigh] *
                                    [icon_scaled_image_rep samplesPerPixel] * sizeof(unsigned char);
            window_info.icon_data = new uint8_t[icon_data_size];
            window_info.icon_size = icon_scaled_size.to_traa_size();
            if (!window_info.icon_data) {
              LOG_ERROR("alloc memory for icon data failed");
              continue;
            }
            memcpy(const_cast<uint8_t *>(window_info.icon_data), [icon_scaled_image_rep bitmapData],
                   icon_data_size);
          }
        }
      }
    }

    // Skip windows that are can not get image.
    NSBitmapImageRep *origin_image_rep =
        get_image_rep(true, static_cast<CGWindowID>(window_info.id));
    if (!origin_image_rep) {
      continue;
    }

    // only get thumbnail for the window when thumbnail_size is set.
    if (thumbnail_size.width > 0 && thumbnail_size.height > 0) {
      NSImage *origin_image = [NSImage new];
      [origin_image addRepresentation:origin_image_rep];
      if (origin_image.size.width <= 1 || origin_image.size.height <= 1) {
        continue;
      }

      desktop_size scaled_size = calc_scaled_size(
          desktop_size(origin_image.size.width, origin_image.size.height), thumbnail_size);
      CGSize scaled_ns_size = CGSizeMake(scaled_size.width(), scaled_size.height());
      NSBitmapImageRep *scaled_image_rep = scale_image(origin_image_rep, scaled_ns_size);
      if (!scaled_image_rep) {
        continue;
      }

#if 0
      dump_image_to_file(
          scaled_image_rep,
          (std::string("thumbnail_") + std::to_string(window_info.id) + ".png").c_str());
#endif

      size_t thumbnail_data_size = [scaled_image_rep pixelsWide] * [scaled_image_rep pixelsHigh] *
                                   [scaled_image_rep samplesPerPixel] * sizeof(unsigned char);
      window_info.thumbnail_data = new uint8_t[thumbnail_data_size];
      window_info.thumbnail_size = scaled_size.to_traa_size();
      if (!window_info.thumbnail_data) {
        LOG_ERROR("alloc memory for thumbnail data failed");
        continue;
      }
      memcpy(const_cast<uint8_t *>(window_info.thumbnail_data), [scaled_image_rep bitmapData],
             thumbnail_data_size);
    }

    // Get screen id owned the window.
    // TODO (@sylar) : The desktop_config.displays is always only contain one which is the current
    // screen. So it seems that to get the screen id is not necessary for macOS.
    if (window_bounds.size.width > 0 && window_bounds.size.height > 0) {
      for (size_t i = 0; i < desktop_config.displays.size(); i++) {
        if (desktop_config.displays[i].bounds.contains(
                desktop_rect::make_xywh(window_bounds.origin.x, window_bounds.origin.y,
                                        window_bounds.size.width, window_bounds.size.height))) {
          window_info.screen_id = static_cast<int>(i);
          break;
        }
      }
    }

    infos.push_back(window_info);
  }

  CFRelease(window_array);
  return TRAA_ERROR_NONE;
}

int enum_screens(const traa_size thumbnail_size, const unsigned int external_flags,
                 std::vector<traa_screen_source_info> &infos) {
  NSArray *screen_array = NSScreen.screens;
  if (!screen_array) {
    return TRAA_ERROR_NONE;
  }

  for (size_t i = 0; i < screen_array.count; i++) {
    NSScreen *screen = screen_array[i];
    if (!screen) {
      continue;
    }

    CGDirectDisplayID screen_id = static_cast<CGDirectDisplayID>(
        [[screen.deviceDescription objectForKey:@"NSScreenNumber"] unsignedIntValue]);
    CGRect screen_bounds = CGDisplayBounds(screen_id);

    NSBitmapImageRep *screen_image_rep = get_image_rep(false, screen_id);
    if (!screen_image_rep) {
      continue;
    }

    traa_screen_source_info screen_info;
    screen_info.is_window = false;
    screen_info.is_primary = CGDisplayIsMain(screen_id);
    screen_info.id = static_cast<int64_t>(screen_id);
    snprintf(const_cast<char *>(screen_info.title), sizeof(screen_info.title), "Display %d",
             static_cast<int>(i));
    screen_info.rect = desktop_rect::make_xywh(screen_bounds.origin.x, screen_bounds.origin.y,
                                               screen_bounds.size.width, screen_bounds.size.height)
                           .to_traa_rect();

    // only get thumbnail for the screen when thumbnail_size is set.
    if (thumbnail_size.width > 0 && thumbnail_size.height > 0) {
      NSImage *screen_image = [NSImage new];
      [screen_image addRepresentation:screen_image_rep];

      desktop_size scaled_size = calc_scaled_size(
          desktop_size(screen_image.size.width, screen_image.size.height), thumbnail_size);
      CGSize scaled_ns_size = CGSizeMake(scaled_size.width(), scaled_size.height());

      NSBitmapImageRep *scaled_image_rep = scale_image(screen_image_rep, scaled_ns_size);
      if (!scaled_image_rep) {
        continue;
      }

#if 0
      dump_image_to_file(
          scaled_image_rep,
          (std::string("thumbnail_") + std::to_string(screen_info.id) + ".png").c_str());
#endif

      size_t thumbnail_data_size = [scaled_image_rep pixelsWide] * [scaled_image_rep pixelsHigh] *
                                   [scaled_image_rep samplesPerPixel] * sizeof(unsigned char);
      screen_info.thumbnail_data = new uint8_t[thumbnail_data_size];
      screen_info.thumbnail_size = scaled_size.to_traa_size();
      if (!screen_info.thumbnail_data) {
        LOG_ERROR("alloc memory for thumbnail data failed");
        continue;
      }
      memcpy(const_cast<uint8_t *>(screen_info.thumbnail_data), [scaled_image_rep bitmapData],
             thumbnail_data_size);
    }

    infos.push_back(screen_info);
  }

  return TRAA_ERROR_NONE;
}

int screen_source_info_enumerator::enum_screen_source_info(const traa_size icon_size,
                                                           const traa_size thumbnail_size,
                                                           const unsigned int external_flags,
                                                           traa_screen_source_info **infos,
                                                           int *count) {
  if (__builtin_available(macOS 11, *)) {
    if (!CGPreflightScreenCaptureAccess()) {
      CGRequestScreenCaptureAccess();
      return TRAA_ERROR_PERMISSION_DENIED;
    }
  }

  std::vector<traa_screen_source_info> sources;

  if (!(external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_WINDOW)) {
    enum_windows(icon_size, thumbnail_size, external_flags, sources);
  }

  if (!(external_flags & TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN)) {
    enum_screens(thumbnail_size, external_flags, sources);
  }

  if (sources.size() == 0) {
    return TRAA_ERROR_NONE;
  }

  *count = static_cast<int>(sources.size());
  *infos = reinterpret_cast<traa_screen_source_info *>(new traa_screen_source_info[sources.size()]);
  if (*infos == nullptr) {
    LOG_ERROR("alloca memroy for infos failed");
    return TRAA_ERROR_OUT_OF_MEMORY;
  }

  for (size_t i = 0; i < sources.size(); ++i) {
    auto &source_info = sources[i];
    auto &dest_info = (*infos)[i];
    memcpy(&dest_info, &source_info, sizeof(traa_screen_source_info));
    if (std::strlen(source_info.title) > 0) {
      strncpy(const_cast<char *>(dest_info.title), source_info.title,
              std::min(sizeof(dest_info.title) - 1, std::strlen(source_info.title)));
    }

    if (std::strlen(source_info.process_path) > 0) {
      strncpy(const_cast<char *>(dest_info.process_path), source_info.process_path,
              std::min(sizeof(dest_info.process_path) - 1, std::strlen(source_info.process_path)));
    }
  }

  return TRAA_ERROR_NONE;
}

int screen_source_info_enumerator::create_snapshot(const int64_t source_id,
                                                   const traa_size snapshot_size, uint8_t **data,
                                                   int *data_size, traa_size *actual_size) {
  if (source_id < 0 || data == nullptr || data_size == nullptr || actual_size == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (__builtin_available(macOS 11, *)) {
    if (!CGPreflightScreenCaptureAccess()) {
      CGRequestScreenCaptureAccess();
      return TRAA_ERROR_PERMISSION_DENIED;
    }
  }

  bool is_window = false;
  if (!is_source_id_valid(source_id, is_window)) {
    return TRAA_ERROR_INVALID_SOURCE_ID;
  }

  NSBitmapImageRep *image_rep = get_image_rep(is_window, static_cast<CGWindowID>(source_id));
  if (!image_rep) {
    return TRAA_ERROR_INVALID_SOURCE_ID;
  }

  NSImage *image = [NSImage new];
  [image addRepresentation:image_rep];

  desktop_size scaled_size =
      calc_scaled_size(desktop_size(image.size.width, image.size.height), snapshot_size);
  CGSize scaled_ns_size = CGSizeMake(scaled_size.width(), scaled_size.height());
  NSBitmapImageRep *scaled_image_rep = scale_image(image_rep, scaled_ns_size);
  if (!scaled_image_rep) {
    return TRAA_ERROR_INVALID_SOURCE_ID;
  }

  size_t thumbnail_data_size = [scaled_image_rep pixelsWide] * [scaled_image_rep pixelsHigh] *
                               [scaled_image_rep samplesPerPixel] * sizeof(unsigned char);

  *data = new uint8_t[thumbnail_data_size];
  *data_size = static_cast<int>(thumbnail_data_size);
  if (*data == nullptr) {
    LOG_ERROR("alloc memory for data failed");
    return TRAA_ERROR_OUT_OF_MEMORY;
  }

  memcpy(*data, [scaled_image_rep bitmapData], thumbnail_data_size);
  *actual_size = scaled_size.to_traa_size();

  return TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa
