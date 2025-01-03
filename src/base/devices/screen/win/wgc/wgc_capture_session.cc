/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/wgc/wgc_capture_session.h"

#include "base/devices/screen/win/wgc/wgc_desktop_frame.h"
#include "base/logger.h"
#include "base/system/sleep.h"
#include "base/utils/time_utils.h"
#include "base/utils/win/create_direct3d_device.h"
#include "base/utils/win/get_activation_factory.h"

#include <DispatcherQueue.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directX.direct3d11.interop.h>
#include <windows.graphics.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

using Microsoft::WRL::ComPtr;
namespace WGC = ABI::Windows::Graphics::Capture;

namespace traa {
namespace base {

inline namespace {

// We must use a BGRA pixel format that has 4 bytes per pixel, as required by
// the DesktopFrame interface.
constexpr auto k_pixel_format =
    ABI::Windows::Graphics::DirectX::DirectXPixelFormat::DirectXPixelFormat_B8G8R8A8UIntNormalized;

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class start_capture_result {
  success = 0,
  source_closed = 1,
  add_closed_failed = 2,
  dxgi_device_cast_failed = 3,
  d3d_delay_load_failed = 4,
  d3d_device_creation_failed = 5,
  frame_pool_activation_failed = 6,
  // frame_pool_cast_failed = 7, (deprecated)
  // get_item_size_failed = 8, (deprecated)
  create_frame_pool_failed = 9,
  create_capture_session_failed = 10,
  start_capture_failed = 11,
  max_value = start_capture_failed
};

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class get_frame_result {
  success = 0,
  item_closed = 1,
  try_get_next_frame_failed = 2,
  frame_dropped = 3,
  get_surface_failed = 4,
  dxgi_interface_access_failed = 5,
  texture2d_cast_failed = 6,
  create_mapped_texture_failed = 7,
  map_frame_failed = 8,
  get_content_size_failed = 9,
  resize_mapped_texture_failed = 10,
  recreate_frame_pool_failed = 11,
  frame_pool_empty = 12,
  max_value = frame_pool_empty
};

void record_start_capture_result(start_capture_result error) {
  LOG_INFO("start_capture_result {}", static_cast<int>(error));
}

void record_get_frame_result(get_frame_result error) {
  LOG_INFO("get_frame_result {}", static_cast<int>(error));
}

bool size_has_changed(ABI::Windows::Graphics::SizeInt32 size_new,
                      ABI::Windows::Graphics::SizeInt32 size_old) {
  return (size_new.Height != size_old.Height || size_new.Width != size_old.Width);
}

} // namespace

wgc_capture_session::wgc_capture_session(ComPtr<ID3D11Device> d3d11_device,
                                         ComPtr<WGC::IGraphicsCaptureItem> item,
                                         ABI::Windows::Graphics::SizeInt32 size)
    : d3d11_device_(std::move(d3d11_device)), item_(std::move(item)), size_(size) {}

wgc_capture_session::~wgc_capture_session() { remove_event_handler(); }

HRESULT wgc_capture_session::start_capture(const desktop_capture_options &options) {
  if (item_closed_) {
    LOG_ERROR("The target source has been closed.");
    record_start_capture_result(start_capture_result::source_closed);
    return E_ABORT;
  }

  // Listen for the Closed event, to detect if the source we are capturing is
  // closed (e.g. application window is closed or monitor is disconnected). If
  // it is, we should abort the capture.
  item_closed_token_ = std::make_unique<EventRegistrationToken>();
  auto closed_handler = Microsoft::WRL::Callback<
      ABI::Windows::Foundation::ITypedEventHandler<WGC::GraphicsCaptureItem *, IInspectable *>>(
      this, &wgc_capture_session::on_item_closed);
  HRESULT hr = item_->add_Closed(closed_handler.Get(), item_closed_token_.get());
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::add_closed_failed);
    return hr;
  }

  ComPtr<IDXGIDevice> dxgi_device;
  hr = d3d11_device_->QueryInterface(IID_PPV_ARGS(&dxgi_device));
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::dxgi_device_cast_failed);
    return hr;
  }

  if (!resolve_core_winrt_direct3d_delayload()) {
    record_start_capture_result(start_capture_result::d3d_delay_load_failed);
    return E_FAIL;
  }

  hr = create_direct3d_device_from_dxgi_device(dxgi_device.Get(), &direct3d_device_);
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::d3d_device_creation_failed);
    return hr;
  }

  ComPtr<WGC::IDirect3D11CaptureFramePoolStatics> frame_pool_statics;
  hr = get_activation_factory<WGC::IDirect3D11CaptureFramePoolStatics,
                              RuntimeClass_Windows_Graphics_Capture_Direct3D11CaptureFramePool>(
      &frame_pool_statics);
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::frame_pool_activation_failed);
    return hr;
  }

  hr = frame_pool_statics->Create(direct3d_device_.Get(), k_pixel_format, k_num_buffers, size_,
                                  &frame_pool_);
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::create_frame_pool_failed);
    return hr;
  }

  hr = frame_pool_->CreateCaptureSession(item_.Get(), &session_);
  if (FAILED(hr)) {
    record_start_capture_result(start_capture_result::create_capture_session_failed);
    return hr;
  }

  if (!options.prefer_cursor_embedded()) {
    ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureSession2> session2;
    if (SUCCEEDED(session_->QueryInterface(
            ABI::Windows::Graphics::Capture::IID_IGraphicsCaptureSession2, &session2))) {
      session2->put_IsCursorCaptureEnabled(false);
    }
  }

  // By default, the WGC capture API adds a yellow border around the captured
  // window or display to indicate that a capture is in progress. The section
  // below is an attempt to remove this yellow border to make the capture
  // experience more inline with the DXGI capture path.
  // This requires 10.0.20348.0 or later, which practically means Windows 11.
  ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureSession3> session3;
  if (SUCCEEDED(session_->QueryInterface(
          ABI::Windows::Graphics::Capture::IID_IGraphicsCaptureSession3, &session3))) {
    session3->put_IsBorderRequired(false);
  }

  allow_zero_hertz_ = options.allow_wgc_zero_hertz();

  hr = session_->StartCapture();
  if (FAILED(hr)) {
    LOG_ERROR("Failed to start CaptureSession: {}", hr);
    record_start_capture_result(start_capture_result::start_capture_failed);
    return hr;
  }

  record_start_capture_result(start_capture_result::success);

  is_capture_started_ = true;
  return hr;
}

void wgc_capture_session::ensure_frame() {
  // Try to process the captured frame and copy it to the `queue_`.
  HRESULT hr = process_frame();
  if (SUCCEEDED(hr)) {
    return;
  }

  // We failed to process the frame, but we do have a frame so just return that.
  if (queue_.current_frame()) {
    LOG_ERROR("process_frame failed, using existing frame: {}", hr);
    return;
  }

  // process_frame failed and we don't have a current frame. This could indicate
  // a startup path where we may need to try/wait a few times to ensure that we
  // have a frame. We try to get a new frame from the frame pool for a maximum
  // of 10 times after sleeping for 20ms. We choose 20ms as it's just a bit
  // longer than 17ms (for 60fps*) and hopefully avoids unlucky timing causing
  // us to wait two frames when we mostly seem to only need to wait for one.
  // This approach should ensure that GetFrame() always delivers a valid frame
  // with a max latency of 200ms and often after sleeping only once.
  // The scheme is heuristic and based on manual testing.
  // (*) On a modern system, the FPS / monitor refresh rate is usually larger
  //     than or equal to 60.

  const int max_sleep_count = 10;
  const int sleep_time_ms = 20;

  int sleep_count = 0;
  while (!queue_.current_frame() && sleep_count < max_sleep_count) {
    sleep_count++;
    sleep_ms(sleep_time_ms);
    hr = process_frame();
    if (FAILED(hr)) {
      LOG_ERROR("process_frame failed during startup: {}", hr);
    }
  }
  if (!queue_.current_frame()) {
    LOG_ERROR("uanble to process a valid frame even after trying 10 times.");
  }
}

bool wgc_capture_session::get_frame(std::unique_ptr<desktop_frame> *output_frame,
                                    bool source_should_be_capturable) {
  if (item_closed_) {
    LOG_ERROR("The target source has been closed.");
    record_get_frame_result(get_frame_result::item_closed);
    return false;
  }

  // Try to process the captured frame and wait some if needed. Avoid trying
  // if we know that the source will not be capturable. This can happen e.g.
  // when captured window is minimized and if ensure_frame() was called in this
  // state a large amount of kFrameDropped errors would be logged.
  if (source_should_be_capturable)
    ensure_frame();

  // Return a NULL frame and false as `result` if we still don't have a valid
  // frame. This will lead to a DesktopCapturer::Result::ERROR_PERMANENT being
  // posted by the WGC capturer.
  desktop_frame *current_frame = queue_.current_frame();
  if (!current_frame) {
    LOG_ERROR("get_frame failed.");
    return false;
  }

  // Swap in the desktop_region in `damage_region_` which is updated in
  // process_frame(). The updated region is either empty or the full rect being
  // captured where an empty damage region corresponds to "no change in content
  // since last frame".
  current_frame->mutable_updated_region()->swap(&damage_region_);
  damage_region_.clear();

  // Emit the current frame.
  std::unique_ptr<desktop_frame> new_frame = queue_.current_frame()->share();
  *output_frame = std::move(new_frame);

  return true;
}

HRESULT wgc_capture_session::create_mapped_texture(ComPtr<ID3D11Texture2D> src_texture, UINT width,
                                                   UINT height) {
  D3D11_TEXTURE2D_DESC src_desc;
  src_texture->GetDesc(&src_desc);
  D3D11_TEXTURE2D_DESC map_desc;
  map_desc.Width = width == 0 ? src_desc.Width : width;
  map_desc.Height = height == 0 ? src_desc.Height : height;
  map_desc.MipLevels = src_desc.MipLevels;
  map_desc.ArraySize = src_desc.ArraySize;
  map_desc.Format = src_desc.Format;
  map_desc.SampleDesc = src_desc.SampleDesc;
  map_desc.Usage = D3D11_USAGE_STAGING;
  map_desc.BindFlags = 0;
  map_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  map_desc.MiscFlags = 0;
  return d3d11_device_->CreateTexture2D(&map_desc, nullptr, &mapped_texture_);
}

HRESULT wgc_capture_session::process_frame() {
  ComPtr<WGC::IDirect3D11CaptureFrame> capture_frame;
  HRESULT hr = frame_pool_->TryGetNextFrame(&capture_frame);
  if (FAILED(hr)) {
    LOG_ERROR("TryGetNextFrame failed: {}", hr);
    record_get_frame_result(get_frame_result::try_get_next_frame_failed);
    return hr;
  }

  if (!capture_frame) {
    if (!queue_.current_frame()) {
      // The frame pool was empty and so is the external queue.
      LOG_ERROR("Frame pool was empty => kFrameDropped.");
      record_get_frame_result(get_frame_result::frame_dropped);
    } else {
      // The frame pool was empty but there is still one old frame available in
      // external the queue.
      LOG_ERROR("Frame pool was empty => kFramePoolEmpty.");
      record_get_frame_result(get_frame_result::frame_pool_empty);
    }
    return E_FAIL;
  }

  queue_.move_to_next_frame();
  if (queue_.current_frame() && queue_.current_frame()->is_shared()) {
    LOG_ERROR("Overwriting frame that is still shared.");
  }

  // We need to get `capture_frame` as an `ID3D11Texture2D` so that we can get
  // the raw image data in the format required by the `DesktopFrame` interface.
  ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface> d3d_surface;
  hr = capture_frame->get_Surface(&d3d_surface);
  if (FAILED(hr)) {
    record_get_frame_result(get_frame_result::get_surface_failed);
    return hr;
  }

  ComPtr<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>
      direct3DDxgiInterfaceAccess;
  hr = d3d_surface->QueryInterface(IID_PPV_ARGS(&direct3DDxgiInterfaceAccess));
  if (FAILED(hr)) {
    record_get_frame_result(get_frame_result::dxgi_interface_access_failed);
    return hr;
  }

  ComPtr<ID3D11Texture2D> texture_2D;
  hr = direct3DDxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&texture_2D));
  if (FAILED(hr)) {
    record_get_frame_result(get_frame_result::texture2d_cast_failed);
    return hr;
  }

  if (!mapped_texture_) {
    hr = create_mapped_texture(texture_2D);
    if (FAILED(hr)) {
      record_get_frame_result(get_frame_result::create_mapped_texture_failed);
      return hr;
    }
  }

  // We need to copy `texture_2D` into `mapped_texture_` as the latter has the
  // D3D11_CPU_ACCESS_READ flag set, which lets us access the image data.
  // Otherwise it would only be readable by the GPU.
  ComPtr<ID3D11DeviceContext> d3d_context;
  d3d11_device_->GetImmediateContext(&d3d_context);

  ABI::Windows::Graphics::SizeInt32 new_size;
  hr = capture_frame->get_ContentSize(&new_size);
  if (FAILED(hr)) {
    record_get_frame_result(get_frame_result::get_content_size_failed);
    return hr;
  }

  // If the size changed, we must resize `mapped_texture_` and `frame_pool_` to
  // fit the new size. This must be done before `CopySubresourceRegion` so that
  // the textures are the same size.
  if (size_has_changed(new_size, size_)) {
    hr = create_mapped_texture(texture_2D, new_size.Width, new_size.Height);
    if (FAILED(hr)) {
      record_get_frame_result(get_frame_result::resize_mapped_texture_failed);
      return hr;
    }

    hr = frame_pool_->Recreate(direct3d_device_.Get(), k_pixel_format, k_num_buffers, new_size);
    if (FAILED(hr)) {
      record_get_frame_result(get_frame_result::recreate_frame_pool_failed);
      return hr;
    }
  }

  // If the size has changed since the last capture, we must be sure to use
  // the smaller dimensions. Otherwise we might overrun our buffer, or
  // read stale data from the last frame.
  int image_height = std::min(size_.Height, new_size.Height);
  int image_width = std::min(size_.Width, new_size.Width);

  D3D11_BOX copy_region;
  copy_region.left = 0;
  copy_region.top = 0;
  copy_region.right = image_width;
  copy_region.bottom = image_height;
  // Our textures are 2D so we just want one "slice" of the box.
  copy_region.front = 0;
  copy_region.back = 1;
  d3d_context->CopySubresourceRegion(mapped_texture_.Get(),
                                     /*dst_subresource_index=*/0, /*dst_x=*/0,
                                     /*dst_y=*/0, /*dst_z=*/0, texture_2D.Get(),
                                     /*src_subresource_index=*/0, &copy_region);

  D3D11_MAPPED_SUBRESOURCE map_info;
  hr = d3d_context->Map(mapped_texture_.Get(), /*subresource_index=*/0, D3D11_MAP_READ,
                        /*D3D11_MAP_FLAG_DO_NOT_WAIT=*/0, &map_info);
  if (FAILED(hr)) {
    record_get_frame_result(get_frame_result::map_frame_failed);
    return hr;
  }

  // Allocate the current frame buffer only if it is not already allocated or
  // if the size has changed. Note that we can't reallocate other buffers at
  // this point, since the caller may still be reading from them. The queue can
  // hold up to two frames.
  desktop_size image_size(image_width, image_height);
  if (!queue_.current_frame() || !queue_.current_frame()->size().equals(image_size)) {
    std::unique_ptr<desktop_frame> buffer = std::make_unique<basic_desktop_frame>(image_size);
    queue_.replace_current_frame(shared_desktop_frame::wrap(std::move(buffer)));
  }

  desktop_frame *current_frame = queue_.current_frame();
  desktop_frame *previous_frame = queue_.previous_frame();

  // Will be set to true while copying the frame data to the `current_frame` if
  // we can already determine that the content of the new frame differs from the
  // previous. The idea is to get a low-complexity indication of if the content
  // is static or not without performing a full/deep memory comparison when
  // updating the damaged region.
  bool frame_content_has_changed = false;

  // Check if the queue contains two frames whose content can be compared.
  const bool can_be_compared = frame_content_can_be_compared();

  // Make a copy of the data pointed to by `map_info.pData` to the preallocated
  // `current_frame` so we are free to unmap our texture. If possible, also
  // perform a light-weight scan of the vertical line of pixels in the middle
  // of the screen. A comparison is performed between two 32-bit pixels (RGBA);
  // one from the current frame and one from the previous, and as soon as a
  // difference is detected the scan stops and `frame_content_has_changed` is
  // set to true.
  uint8_t *src_data = static_cast<uint8_t *>(map_info.pData);
  uint8_t *dst_data = current_frame->data();
  uint8_t *prev_data = can_be_compared ? previous_frame->data() : nullptr;

  const int width_in_bytes = current_frame->size().width() * desktop_frame::k_bytes_per_pixel;
  const int middle_pixel_offset = (image_width / 2) * desktop_frame::k_bytes_per_pixel;
  for (int i = 0; i < image_height; i++) {
    memcpy(dst_data, src_data, width_in_bytes);
    if (prev_data && !frame_content_has_changed) {
      uint8_t *previous_pixel = prev_data + middle_pixel_offset;
      uint8_t *current_pixel = dst_data + middle_pixel_offset;
      frame_content_has_changed =
          memcmp(previous_pixel, current_pixel, desktop_frame::k_bytes_per_pixel);
      prev_data += current_frame->stride();
    }
    dst_data += current_frame->stride();
    src_data += map_info.RowPitch;
  }

  d3d_context->Unmap(mapped_texture_.Get(), 0);

  if (allow_zero_hertz()) {
    if (previous_frame) {
      const int previous_frame_size = previous_frame->stride() * previous_frame->size().height();
      const int current_frame_size = current_frame->stride() * current_frame->size().height();

      // Compare the latest frame with the previous and check if the frames are
      // equal (both contain the exact same pixel values). Avoid full memory
      // comparison if indication of a changed frame already exists from the
      // stage above.
      if (current_frame_size == previous_frame_size) {
        if (frame_content_has_changed) {
          // Mark frame as damaged based on existing light-weight indicator.
          // Avoids deep memcmp of complete frame and saves resources.
          damage_region_.set_rect(desktop_rect::make_size(current_frame->size()));
        } else {
          // Perform full memory comparison for all bytes between the current
          // and the previous frames.
          const bool frames_are_equal =
              !memcmp(current_frame->data(), previous_frame->data(), current_frame_size);
          if (!frames_are_equal) {
            // TODO(https://crbug.com/1421242): If we had an API to report
            // proper damage regions we should be doing AddRect() with a
            // SetRect() call on a resize.
            damage_region_.set_rect(desktop_rect::make_size(current_frame->size()));
          }
        }
      } else {
        // Mark resized frames as damaged.
        damage_region_.set_rect(desktop_rect::make_size(current_frame->size()));
      }
    }
  }

  size_ = new_size;
  record_get_frame_result(get_frame_result::success);
  return hr;
}

HRESULT wgc_capture_session::on_item_closed(WGC::IGraphicsCaptureItem *sender,
                                            IInspectable *event_args) {
  LOG_INFO("Capture target has been closed.");
  item_closed_ = true;

  remove_event_handler();

  // Do not attempt to free resources in the OnItemClosed handler, as this
  // causes a race where we try to delete the item that is calling us. Removing
  // the event handlers and setting `item_closed_` above is sufficient to ensure
  // that the resources are no longer used, and the next time the capturer tries
  // to get a frame, we will report a permanent failure and be destroyed.
  return S_OK;
}

void wgc_capture_session::remove_event_handler() {
  HRESULT hr;
  if (item_ && item_closed_token_) {
    hr = item_->remove_Closed(*item_closed_token_);
    item_closed_token_.reset();
    if (FAILED(hr))
      LOG_WARN("Failed to remove Closed event handler: {}", hr);
  }
}

bool wgc_capture_session::frame_content_can_be_compared() {
  desktop_frame *current_frame = queue_.current_frame();
  desktop_frame *previous_frame = queue_.previous_frame();
  if (!current_frame || !previous_frame) {
    return false;
  }
  if (current_frame->stride() != previous_frame->stride()) {
    return false;
  }
  return current_frame->size().equals(previous_frame->size());
}

} // namespace base
} // namespace traa
