/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/camera/device_info_impl.h"

#include "base/logger.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#ifndef abs
#define abs(a) (a >= 0 ? a : -a)
#endif

#ifdef _WIN32
#define str_case_cmp _stricmp
#else
#define str_case_cmp strcasecmp
#endif

namespace traa {
namespace base {

device_info_impl::device_info_impl()
    : last_used_device_name_(nullptr), last_used_device_name_length_(0) {}

device_info_impl::~device_info_impl() {
  std::lock_guard<std::mutex> lock(api_lock_);
  free(last_used_device_name_);
}

int32_t device_info_impl::number_of_capabilities(const char *device_unique_id_utf8) {
  if (!device_unique_id_utf8)
    return -1;

  std::lock_guard<std::mutex> lock(api_lock_);

  // Is it the same device that is asked for again.
  if (last_used_device_name_ && last_used_device_name_length_ > 0 &&
      str_case_cmp(device_unique_id_utf8, last_used_device_name_) == 0) {
    return static_cast<int32_t>(capture_capabilities_.size());
  }

  int32_t ret = create_capability_map(device_unique_id_utf8);
  return ret;
}

int32_t device_info_impl::get_capability(const char *device_unique_id_utf8,
                                         uint32_t device_capability_number,
                                         video_capture_capability &capability) {
  assert(device_unique_id_utf8);

  std::lock_guard<std::mutex> lock(api_lock_);

  if (!last_used_device_name_ || last_used_device_name_length_ == 0 ||
      str_case_cmp(device_unique_id_utf8, last_used_device_name_) != 0) {
    if (-1 == create_capability_map(device_unique_id_utf8)) {
      return -1;
    }
  }

  // Make sure the number is valid.
  if (device_capability_number >= static_cast<unsigned int>(capture_capabilities_.size())) {
    LOG_ERROR("invalid device_capability_number {} >= number of capabilities ({}).",
              device_capability_number, capture_capabilities_.size());
    return -1;
  }

  capability = capture_capabilities_[device_capability_number];
  return 0;
}

int32_t device_info_impl::get_best_matched_capability(const char *device_unique_id_utf8,
                                                      const video_capture_capability &requested,
                                                      video_capture_capability &resulting) {
  if (!device_unique_id_utf8)
    return -1;

  std::lock_guard<std::mutex> lock(api_lock_);
  if (!last_used_device_name_ || last_used_device_name_length_ == 0 ||
      str_case_cmp(device_unique_id_utf8, last_used_device_name_) != 0) {
    if (-1 == create_capability_map(device_unique_id_utf8)) {
      return -1;
    }
  }

  int32_t best_format_index = -1;
  int32_t best_width = 0;
  int32_t best_height = 0;
  int32_t best_frame_rate = 0;
  video_type best_video_type = video_type::k_unknown;

  const int32_t number_of_capabilities =
      static_cast<int32_t>(capture_capabilities_.size());

  for (int32_t tmp = 0; tmp < number_of_capabilities; ++tmp) {
    video_capture_capability &capability = capture_capabilities_[tmp];

    const int32_t diff_width = capability.width - requested.width;
    const int32_t diff_height = capability.height - requested.height;
    const int32_t diff_frame_rate = capability.max_fps - requested.max_fps;

    const int32_t current_best_diff_width = best_width - requested.width;
    const int32_t current_best_diff_height = best_height - requested.height;
    const int32_t current_best_diff_frame_rate = best_frame_rate - requested.max_fps;

    if ((diff_height >= 0 &&
         diff_height <= abs(current_best_diff_height)) // Height better or equal than previous.
        || (current_best_diff_height < 0 && diff_height >= current_best_diff_height)) {
      if (diff_height == current_best_diff_height) // Found best height. Care about the width.
      {
        if ((diff_width >= 0 &&
             diff_width <= abs(current_best_diff_width)) // Width better or equal.
            || (current_best_diff_width < 0 && diff_width >= current_best_diff_width)) {
          if (diff_width == current_best_diff_width &&
              diff_height == current_best_diff_height) // Same size as previous.
          {
            // Also check the best frame rate if the diff is the same as previous.
            if (((diff_frame_rate >= 0 &&
                  diff_frame_rate <=
                      current_best_diff_frame_rate) // Frame rate too high but better match.
                 || (current_best_diff_frame_rate < 0 &&
                     diff_frame_rate >=
                         current_best_diff_frame_rate)) // Current frame rate is lower than
                                                        // requested. This is better.
            ) {
              if ((current_best_diff_frame_rate ==
                   diff_frame_rate) // Same frame rate as previous or frame rate already good enough.
                  || (current_best_diff_frame_rate >= 0)) {
                if (best_video_type != requested.video_type &&
                    requested.video_type != video_type::k_unknown &&
                    (capability.video_type == requested.video_type ||
                     capability.video_type == video_type::k_i420 ||
                     capability.video_type == video_type::k_yuy2 ||
                     capability.video_type == video_type::k_yv12 ||
                     capability.video_type == video_type::k_nv12)) {
                  best_video_type = capability.video_type;
                  best_format_index = tmp;
                }
                // If width, height, and frame rate are fulfilled we can use the camera for
                // encoding if it is supported.
                if (capability.height == requested.height &&
                    capability.width == requested.width &&
                    capability.max_fps >= requested.max_fps) {
                  best_video_type = capability.video_type;
                  best_format_index = tmp;
                }
              } else // Better frame rate.
              {
                best_width = capability.width;
                best_height = capability.height;
                best_frame_rate = capability.max_fps;
                best_video_type = capability.video_type;
                best_format_index = tmp;
              }
            }
          } else // Better width than previous.
          {
            best_width = capability.width;
            best_height = capability.height;
            best_frame_rate = capability.max_fps;
            best_video_type = capability.video_type;
            best_format_index = tmp;
          }
        }     // else width no good
      } else  // Better height.
      {
        best_width = capability.width;
        best_height = capability.height;
        best_frame_rate = capability.max_fps;
        best_video_type = capability.video_type;
        best_format_index = tmp;
      }
    } // else height not good
  }   // end for

  LOG_INFO("best camera format: {}x{}@{}fps, color format: {}", best_width, best_height,
           best_frame_rate, static_cast<int>(best_video_type));

  // Copy the capability.
  if (best_format_index < 0)
    return -1;
  resulting = capture_capabilities_[best_format_index];
  return best_format_index;
}

} // namespace base
} // namespace traa
