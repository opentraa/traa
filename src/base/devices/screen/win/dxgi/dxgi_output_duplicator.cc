/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_output_duplicator.h"

#include "base/devices/screen/win/desktop_capture_utils.h"
#include "base/devices/screen/win/dxgi/dxgi_texture_mapping.h"
#include "base/devices/screen/win/dxgi/dxgi_texture_staging.h"

#include "base/logger.h"
#include "base/strings/string_trans.h"

#include <dxgi.h>
#include <dxgiformat.h>
#include <string.h>
#include <unknwn.h>
#include <windows.h>

#include <algorithm>

namespace traa {
namespace base {
using Microsoft::WRL::ComPtr;

namespace {

// Timeout for AcquireNextFrame() call.
// dxgi_duplicator_controller leverages external components to do the capture
// scheduling. So here dxgi_output_duplicator does not need to actively wait for a
// new frame.
const int k_acquire_timeout_ms = 0;

desktop_rect rect_to_desktop_rect(const RECT &rect) {
  return desktop_rect::make_ltrb(rect.left, rect.top, rect.right, rect.bottom);
}

rotation dxgi_rotation_to_rotation(DXGI_MODE_ROTATION rotation) {
  switch (rotation) {
  case DXGI_MODE_ROTATION_IDENTITY:
  case DXGI_MODE_ROTATION_UNSPECIFIED:
    return rotation::r_0;
  case DXGI_MODE_ROTATION_ROTATE90:
    return rotation::r_90;
  case DXGI_MODE_ROTATION_ROTATE180:
    return rotation::r_180;
  case DXGI_MODE_ROTATION_ROTATE270:
    return rotation::r_270;
  }

  return rotation::r_0;
}

} // namespace

dxgi_output_duplicator::dxgi_output_duplicator(const d3d_device &device,
                                               const ComPtr<IDXGIOutput1> &output,
                                               const DXGI_OUTPUT_DESC &desc)
    : device_(device), output_(output),
      device_name_(string_trans::unicode_to_utf8(desc.DeviceName)),
      desktop_rect_(rect_to_desktop_rect(desc.DesktopCoordinates)) {}

dxgi_output_duplicator::dxgi_output_duplicator(dxgi_output_duplicator &&other) = default;

dxgi_output_duplicator::~dxgi_output_duplicator() {
  if (duplication_) {
    duplication_->ReleaseFrame();
  }
  texture_.reset();
}

bool dxgi_output_duplicator::initialize() {
  if (duplicate_output()) {
    if (desc_.DesktopImageInSystemMemory) {
      texture_.reset(new dxgi_texture_mapping(duplication_.Get()));
    } else {
      texture_.reset(new dxgi_texture_staging(device_));
    }
    return true;
  } else {
    duplication_.Reset();
    return false;
  }
}

bool dxgi_output_duplicator::duplicate_output() {
  _com_error error = output_->DuplicateOutput(static_cast<IUnknown *>(device_.get_d3d_device()),
                                              duplication_.GetAddressOf());
  if (error.Error() != S_OK || !duplication_) {
    LOG_ERROR("failed to duplicate output from IDXGIOutput1: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  memset(&desc_, 0, sizeof(desc_));
  duplication_->GetDesc(&desc_);

  // DXGI_FORMAT_R16G16B16A16_FLOAT is returned for HDR monitor,
  // DXGI_FORMAT_B8G8R8A8_UNORM for others.
  if ((desc_.ModeDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) &&
      (desc_.ModeDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)) {
    LOG_ERROR("IDXGIDuplicateOutput does not use RGBA (8, 16 bit) which is required by downstream "
              "components, format is {}",
              static_cast<int>(desc_.ModeDesc.Format));
    return false;
  }

  if (static_cast<int>(desc_.ModeDesc.Width) != desktop_rect_.width() ||
      static_cast<int>(desc_.ModeDesc.Height) != desktop_rect_.height()) {
    LOG_ERROR("IDXGIDuplicateOutput does not return a same size as its IDXGIOutput1, size returned "
              "by IDXGIDuplicateOutput is {} x {}, size returned by IDXGIOutput1 is {} x {}",
              desc_.ModeDesc.Width, desc_.ModeDesc.Height, desktop_rect_.width(),
              desktop_rect_.height());
    return false;
  }

  rotation_ = dxgi_rotation_to_rotation(desc_.Rotation);
  unrotated_size_ = rotate_size(desktop_size(), reverse_rotation(rotation_));

  return true;
}

bool dxgi_output_duplicator::release_frame() {
  _com_error error = duplication_->ReleaseFrame();
  if (error.Error() != S_OK) {
    LOG_ERROR("failed to release frame from IDXGIOutputDuplication: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }
  return true;
}

bool dxgi_output_duplicator::contains_mouse_cursor(const DXGI_OUTDUPL_FRAME_INFO &frame_info) {
  // The DXGI_OUTDUPL_POINTER_POSITION structure that describes the most recent
  // mouse position is only valid if the LastMouseUpdateTime member is a non-
  // zero value.
  if (frame_info.LastMouseUpdateTime.QuadPart == 0)
    return false;

  // Ignore cases when the mouse shape has changed and not the position.
  const bool new_pointer_shape = (frame_info.PointerShapeBufferSize != 0);
  if (new_pointer_shape)
    return false;

  // The mouse cursor has moved and we can now query if the mouse pointer is
  // drawn onto the desktop image or not to decide if we must draw the mouse
  // pointer shape onto the desktop image (always done by default currently).
  // Either the mouse pointer is already drawn onto the desktop image that
  // IDXGIOutputDuplication::AcquireNextFrame provides or the mouse pointer is
  // separate from the desktop image. If the mouse pointer is drawn onto the
  // desktop image, the pointer position data that is reported by
  // AcquireNextFrame indicates that a separate pointer isn’t visible, hence
  // `frame_info.PointerPosition.Visible` is false.
  const bool cursor_embedded_in_frame = !frame_info.PointerPosition.Visible;
  LOG_EVENT_COND("SDM", cursor_embedded_in_frame, "dxgi_output_duplicator.contains_mouse_cursor",
                 cursor_embedded_in_frame);
  return cursor_embedded_in_frame;
}

bool dxgi_output_duplicator::duplicate(context_t *context, desktop_vector offset,
                                       shared_desktop_frame *target) {
  if (!desktop_rect::make_size(target->size()).contains(get_translated_desktop_rect(offset))) {
    // target size is not large enough to cover current output region.
    return false;
  }

  DXGI_OUTDUPL_FRAME_INFO frame_info;
  memset(&frame_info, 0, sizeof(frame_info));
  ComPtr<IDXGIResource> resource;
  _com_error error =
      duplication_->AcquireNextFrame(k_acquire_timeout_ms, &frame_info, resource.GetAddressOf());
  if (error.Error() != S_OK && error.Error() != DXGI_ERROR_WAIT_TIMEOUT) {
    LOG_ERROR("failed to capture frame: {}", desktop_capture_utils::com_error_to_string(error));
    return false;
  }

  const bool cursor_embedded_in_frame = contains_mouse_cursor(frame_info);

  // We need to merge updated region with the one from context, but only spread
  // updated region from current frame. So keeps a copy of updated region from
  // context here. The `updated_region` always starts from (0, 0).
  desktop_region updated_region;
  updated_region.swap(&context->updated_region);
  if (error.Error() == S_OK && frame_info.AccumulatedFrames > 0 && resource) {
    detect_updated_region(frame_info, &context->updated_region);
    spread_context_change(context);
    if (!texture_->copy_from(frame_info, resource.Get())) {
      return false;
    }
    updated_region.add_region(context->updated_region);
    // TODO(zijiehe): Figure out why clearing context->updated_region() here
    // triggers screen flickering?

    const desktop_frame &source = texture_->as_desktop_frame();
    if (rotation_ != rotation::r_0) {
      for (desktop_region::iterator it(updated_region); !it.is_at_end(); it.advance()) {
        // The `updated_region` returned by Windows is rotated, but the `source`
        // frame is not. So we need to rotate it reversely.
        const desktop_rect source_rect =
            rotate_rect(it.rect(), desktop_size(), reverse_rotation(rotation_));
        rotate_desktop_frame(source, source_rect, rotation_, offset, target);
      }
    } else {
      for (desktop_region::iterator it(updated_region); !it.is_at_end(); it.advance()) {
        // The desktop_rect in `target`, starts from offset.
        desktop_rect dest_rect = it.rect();
        dest_rect.translate(offset);
        target->copy_pixels_from(source, it.rect().top_left(), dest_rect);
      }
    }
    last_frame_ = target->share();
    last_frame_offset_ = offset;
    updated_region.translate(offset.x(), offset.y());
    target->mutable_updated_region()->add_region(updated_region);
    target->set_may_contain_cursor(cursor_embedded_in_frame);
    num_frames_captured_++;
    return texture_->release() && release_frame();
  }

  if (last_frame_) {
    // No change since last frame or AcquireNextFrame() timed out, we will
    // export last frame to the target.
    for (desktop_region::iterator it(updated_region); !it.is_at_end(); it.advance()) {
      // The desktop_rect in `source`, starts from last_frame_offset_.
      desktop_rect source_rect = it.rect();
      // The desktop_rect in `target`, starts from offset.
      desktop_rect target_rect = source_rect;
      source_rect.translate(last_frame_offset_);
      target_rect.translate(offset);
      target->copy_pixels_from(*last_frame_, source_rect.top_left(), target_rect);
    }
    updated_region.translate(offset.x(), offset.y());
    target->mutable_updated_region()->add_region(updated_region);
    target->set_may_contain_cursor(cursor_embedded_in_frame);
  } else {
    // If we were at the very first frame, and capturing failed, the
    // context->updated_region should be kept unchanged for next attempt.
    context->updated_region.swap(&updated_region);
  }
  // If AcquireNextFrame() failed with timeout error, we do not need to release
  // the frame.
  return error.Error() == DXGI_ERROR_WAIT_TIMEOUT || release_frame();
}

desktop_rect dxgi_output_duplicator::get_translated_desktop_rect(desktop_vector offset) const {
  desktop_rect result(desktop_rect::make_size(desktop_size()));
  result.translate(offset);
  return result;
}

desktop_rect dxgi_output_duplicator::get_untranslated_desktop_rect() const {
  return desktop_rect::make_size(desktop_size());
}

void dxgi_output_duplicator::detect_updated_region(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                                   desktop_region *updated_region) {
  if (do_detect_updated_region(frame_info, updated_region)) {
    // Make sure even a region returned by Windows API is out of the scope of
    // desktop_rect_, we still won't export it to the target DesktopFrame.
    updated_region->intersect_with(get_untranslated_desktop_rect());
  } else {
    updated_region->set_rect(get_untranslated_desktop_rect());
  }
}

bool dxgi_output_duplicator::do_detect_updated_region(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                                      desktop_region *updated_region) {
  updated_region->clear();
  if (frame_info.TotalMetadataBufferSize == 0) {
    // This should not happen, since frame_info.AccumulatedFrames > 0.
    LOG_ERROR("frame_info.AccumulatedFrames > 0, but TotalMetadataBufferSize == 0");
    return false;
  }

  if (metadata_.size() < frame_info.TotalMetadataBufferSize) {
    metadata_.clear(); // Avoid data copy
    metadata_.resize(frame_info.TotalMetadataBufferSize);
  }

  UINT buff_size = 0;
  DXGI_OUTDUPL_MOVE_RECT *move_rects = reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT *>(metadata_.data());
  size_t move_rects_count = 0;
  _com_error error =
      duplication_->GetFrameMoveRects(static_cast<UINT>(metadata_.size()), move_rects, &buff_size);
  if (error.Error() != S_OK) {
    LOG_ERROR("Failed to get move rectangles: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }
  move_rects_count = buff_size / sizeof(DXGI_OUTDUPL_MOVE_RECT);

  RECT *dirty_rects = reinterpret_cast<RECT *>(metadata_.data() + buff_size);
  size_t dirty_rects_count = 0;
  error = duplication_->GetFrameDirtyRects(static_cast<UINT>(metadata_.size()) - buff_size,
                                           dirty_rects, &buff_size);
  if (error.Error() != S_OK) {
    LOG_ERROR("Failed to get dirty rectangles: {}",
              desktop_capture_utils::com_error_to_string(error));
    return false;
  }
  dirty_rects_count = buff_size / sizeof(RECT);

  while (move_rects_count > 0) {
    // DirectX capturer API may randomly return unmoved move_rects, which should
    // be skipped to avoid unnecessary wasting of differing and encoding
    // resources.
    // By using testing application it2me_standalone_host_main, this check
    // reduces average capture time by 0.375% (4.07 -> 4.055), and average
    // encode time by 0.313% (8.042 -> 8.016) without other impacts.
    if (move_rects->SourcePoint.x != move_rects->DestinationRect.left ||
        move_rects->SourcePoint.y != move_rects->DestinationRect.top) {
      updated_region->add_rect(
          rotate_rect(desktop_rect::make_xywh(
                          move_rects->SourcePoint.x, move_rects->SourcePoint.y,
                          move_rects->DestinationRect.right - move_rects->DestinationRect.left,
                          move_rects->DestinationRect.bottom - move_rects->DestinationRect.top),
                      unrotated_size_, rotation_));
      updated_region->add_rect(rotate_rect(
          desktop_rect::make_ltrb(move_rects->DestinationRect.left, move_rects->DestinationRect.top,
                                  move_rects->DestinationRect.right,
                                  move_rects->DestinationRect.bottom),
          unrotated_size_, rotation_));
    } else {
      LOG_INFO("Unmoved move_rect detected, [{}, {}] - [{}, {}].", move_rects->DestinationRect.left,
               move_rects->DestinationRect.top, move_rects->DestinationRect.right,
               move_rects->DestinationRect.bottom);
    }
    move_rects++;
    move_rects_count--;
  }

  while (dirty_rects_count > 0) {
    updated_region->add_rect(
        rotate_rect(desktop_rect::make_ltrb(dirty_rects->left, dirty_rects->top, dirty_rects->right,
                                            dirty_rects->bottom),
                    unrotated_size_, rotation_));
    dirty_rects++;
    dirty_rects_count--;
  }

  return true;
}

void dxgi_output_duplicator::setup(context_t *context) {
  // Always copy entire monitor during the first Duplicate() function call.
  context->updated_region.add_rect(get_untranslated_desktop_rect());
  contexts_.push_back(context);
}

void dxgi_output_duplicator::unregister(const context_t *const context) {
  auto it = std::find(contexts_.begin(), contexts_.end(), context);
  contexts_.erase(it);
}

void dxgi_output_duplicator::spread_context_change(const context_t *const source) {
  for (context_t *dest : contexts_) {
    if (dest != source) {
      dest->updated_region.add_region(source->updated_region);
    }
  }
}

desktop_size dxgi_output_duplicator::get_desktop_size() const { return desktop_rect_.size(); }

int64_t dxgi_output_duplicator::num_frames_captured() const { return num_frames_captured_; }

void dxgi_output_duplicator::translate_rect(const desktop_vector &position) {
  desktop_rect_.translate(position);
}

} // namespace base
} // namespace traa
