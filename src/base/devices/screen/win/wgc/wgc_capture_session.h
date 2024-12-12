/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURE_SESSION_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURE_SESSION_H_

#include "base/devices/screen/desktop_capture_options.h"
#include "base/devices/screen/screen_capture_frame_queue.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/win/wgc/wgc_capture_source.h"

#include <d3d11.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.h>
#include <wrl/client.h>

#include <memory>

namespace traa {
namespace base {

class wgc_capture_session final {
 public:
  wgc_capture_session(
      Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
      Microsoft::WRL::ComPtr<
          ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> item,
      ABI::Windows::Graphics::SizeInt32 size);

  // Disallow copy and assign.
  wgc_capture_session(const wgc_capture_session&) = delete;
  wgc_capture_session& operator=(const wgc_capture_session&) = delete;

  ~wgc_capture_session();

  HRESULT start_capture(const desktop_capture_options& options);

  // Returns a frame from the local frame queue, if any are present.
  bool get_frame(std::unique_ptr<desktop_frame>* output_frame,
                bool source_should_be_capturable);

  bool is_capture_started() const {
    return is_capture_started_;
  }

  // We keep 2 buffers in the frame pool since it results in a good compromise
  // between latency/capture-rate and the rate at which
  // Direct3D11CaptureFramePool.TryGetNextFrame returns NULL and we have to fall
  // back to providing a copy from our external queue instead.
  // We make this public for tests.
  static constexpr int k_num_buffers = 2;

 private:
  // Initializes `mapped_texture_` with the properties of the `src_texture`,
  // overrides the values of some necessary properties like the
  // D3D11_CPU_ACCESS_READ flag. Also has optional parameters for what size
  // `mapped_texture_` should be, if they aren't provided we will use the size
  // of `src_texture`.
  HRESULT create_mapped_texture(
      Microsoft::WRL::ComPtr<ID3D11Texture2D> src_texture,
      UINT width = 0,
      UINT height = 0);

  // Event handler for `item_`'s Closed event.
  HRESULT on_item_closed(
      ABI::Windows::Graphics::Capture::IGraphicsCaptureItem* sender,
      IInspectable* event_args);

  // Wraps calls to ProcessFrame and deals with the uniqe start-up phase
  // ensuring that we always have one captured frame available.
  void ensure_frame();

  // Process the captured frame and copy it to the `queue_`.
  HRESULT process_frame();

  void remove_event_handler();

  bool frame_content_can_be_compared();

  bool allow_zero_hertz() const { return allow_zero_hertz_; }

  std::unique_ptr<EventRegistrationToken> item_closed_token_;

  // A Direct3D11 Device provided by the caller. We use this to create an
  // IDirect3DDevice, and also to create textures that will hold the image data.
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;

  // This item represents what we are capturing, we use it to create the
  // capture session, and also to listen for the Closed event.
  Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>
      item_;

  // The IDirect3DDevice is necessary to instantiate the frame pool.
  Microsoft::WRL::ComPtr<
      ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>
      direct3d_device_;

  // The frame pool is where frames are deposited during capture, we retrieve
  // them from here with TryGetNextFrame().
  Microsoft::WRL::ComPtr<
      ABI::Windows::Graphics::Capture::IDirect3D11CaptureFramePool>
      frame_pool_;

  // This texture holds the final image data. We made it a member so we can
  // reuse it, instead of having to create a new texture every time we grab a
  // frame.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> mapped_texture_;

  // This is the size of `mapped_texture_` and the buffers in `frame_pool_`. We
  // store this as a member so we can compare it to the size of incoming frames
  // and resize if necessary.
  ABI::Windows::Graphics::SizeInt32 size_;

  // The capture session lets us set properties about the capture before it
  // starts such as whether to capture the mouse cursor, and it lets us tell WGC
  // to start capturing frames.
  Microsoft::WRL::ComPtr<
      ABI::Windows::Graphics::Capture::IGraphicsCaptureSession>
      session_;

  // Queue of captured video frames. The queue holds 2 frames and it avoids
  // alloc/dealloc per captured frame. Incoming frames from the internal frame
  // pool are copied to this queue after required processing in ProcessFrame().
  screen_capture_frame_queue<shared_desktop_frame> queue_;

  bool item_closed_ = false;
  bool is_capture_started_ = false;

  // Caches the value of DesktopCaptureOptions.allow_wgc_zero_hertz() in
  // StartCapture(). Adds 0Hz detection in ProcessFrame() when enabled which
  // adds complexity since memcmp() is performed on two successive frames.
  bool allow_zero_hertz_ = false;

  // Tracks damage region updates that were reported since the last time a frame
  // was captured. Currently only supports either the complete rect being
  // captured or an empty region. Will always be empty if `allow_zero_hertz_` is
  // false.
  desktop_region damage_region_;
};

}  // namespace base
}  // namespace traa

#endif  // TRAA_BASE_DEVICES_SCREEN_WIN_WGC_WGC_CAPTURE_SESSION_H_
