/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_OUTPUT_DUPLICATOR_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_OUTPUT_DUPLICATOR_H_

#include "base/devices/screen/desktop_frame_rotation.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/win/d3d_device.h"
#include "base/devices/screen/win/dxgi/dxgi_context.h"
#include "base/devices/screen/win/dxgi/dxgi_texture.h"
#include "base/thread_annotations.h"

#include <comdef.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <memory>
#include <string>
#include <vector>

namespace traa {
namespace base {

// Duplicates the content on one IDXGIOutput, i.e. one monitor attached to one
// video card. None of functions in this class is thread-safe.
class dxgi_output_duplicator {
public:
  using context_t = dxgi_output_context;

  // Creates an instance of dxgi_output_duplicator from a d3d_device and one of its
  // IDXGIOutput1. Caller must maintain the lifetime of device, to make sure it
  // outlives this instance. Only dxgi_adapter_duplicator can create an
  // instance.
  dxgi_output_duplicator(const d3d_device &device,
                         const Microsoft::WRL::ComPtr<IDXGIOutput1> &output,
                         const DXGI_OUTPUT_DESC &desc);

  // To allow this class to work with vector.
  dxgi_output_duplicator(dxgi_output_duplicator &&other);

  // Destructs this instance. We need to make sure texture_ has been released
  // before duplication_.
  ~dxgi_output_duplicator();

  // Initializes duplication_ object.
  bool initialize();

  // Copies the content of current IDXGIOutput to the `target`. To improve the
  // performance, this function copies only regions merged from
  // `context`->updated_region and DetectUpdatedRegion(). The `offset` decides
  // the offset in the `target` where the content should be copied to. i.e. this
  // function copies the content to the rectangle of (offset.x(), offset.y()) to
  // (offset.x() + desktop_rect_.width(), offset.y() + desktop_rect_.height()).
  // Returns false in case of a failure.
  // May retain a reference to `target` so that a "captured" frame can be
  // returned in the event that a new frame is not ready to be captured yet.
  // (Or in other words, if the call to IDXGIOutputDuplication::AcquireNextFrame
  // indicates that there is not yet a new frame, this is usually because no
  // updates have occurred to the frame).
  bool duplicate(context_t *context, desktop_vector offset, shared_desktop_frame *target);

  // Returns the desktop rect covered by this dxgi_output_duplicator.
  desktop_rect get_desktop_rect() const { return desktop_rect_; }

  // Returns the device name from DXGI_OUTPUT_DESC in utf8 encoding.
  const std::string &get_device_name() const { return device_name_; }

  void setup(context_t *context);

  void unregister(const context_t *const context);

  // How many frames have been captured by this DxigOutputDuplicator.
  int64_t num_frames_captured() const;

  // Moves `desktop_rect_`. See DxgiDuplicatorController::TranslateRect().
  void translate_rect(const desktop_vector &position);

private:
  // Calls do_detect_updated_region(). If it fails, this function sets the
  // `updated_region` as entire UntranslatedDesktopRect().
  void detect_updated_region(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                             desktop_region *updated_region);

  // Returns untranslated updated region, which are directly returned by Windows
  // APIs. Returns false in case of a failure.
  bool do_detect_updated_region(const DXGI_OUTDUPL_FRAME_INFO &frame_info,
                                desktop_region *updated_region);

  // Returns true if the mouse cursor is embedded in the captured frame and
  // false if not. Also logs the same boolean as
  // WebRTC.DesktopCapture.Win.DirectXCursorEmbedded UMA.
  bool contains_mouse_cursor(const DXGI_OUTDUPL_FRAME_INFO &frame_info);

  bool release_frame();

  // Initializes duplication_ instance. Expects duplication_ is in empty status.
  // Returns false if system does not support IDXGIOutputDuplication.
  bool duplicate_output();

  // Returns a desktop_rect with the same size of desktop_size(), but translated
  // by offset.
  desktop_rect get_translated_desktop_rect(desktop_vector offset) const;

  // Returns a desktop_rect with the same size of desktop_size(), but starts from
  // (0, 0).
  desktop_rect get_untranslated_desktop_rect() const;

  // Spreads changes from `context` to other registered context_t(s) in
  // contexts_.
  void spread_context_change(const context_t *const context);

  // Returns the size of desktop rectangle current instance representing.
  desktop_size get_desktop_size() const;

  const d3d_device device_;
  const Microsoft::WRL::ComPtr<IDXGIOutput1> output_;
  const std::string device_name_;
  desktop_rect desktop_rect_;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
  DXGI_OUTDUPL_DESC desc_;
  std::vector<uint8_t> metadata_;
  std::unique_ptr<dxgi_texture> texture_;
  rotation rotation_;
  desktop_size unrotated_size_;

  // After each AcquireNextFrame() function call, updated_region_(s) of all
  // active context_t(s) need to be updated. Since they have missed the
  // change this time. And during next duplicate() function call, their
  // updated_region_ will be merged and copied.
  std::vector<context_t *> contexts_;

  // The last full frame of this output and its offset. If on AcquireNextFrame()
  // failed because of timeout, i.e. no update, we can copy content from
  // `last_frame_`.
  std::unique_ptr<shared_desktop_frame> last_frame_;
  desktop_vector last_frame_offset_;

  int64_t num_frames_captured_ = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_OUTPUT_DUPLICATOR_H_
