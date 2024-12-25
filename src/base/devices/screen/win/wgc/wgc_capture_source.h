/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_WGC_CAPTURE_SOURCE_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_WGC_CAPTURE_SOURCE_H_

#include "base/devices/screen/desktop_capturer.h"
#include "base/devices/screen/desktop_geometry.h"

#include <windows.graphics.capture.h>
#include <windows.graphics.h>
#include <wrl/client.h>

#include <memory>
#include <optional>

namespace traa {
namespace base {

// Abstract class to represent the source that WGC-based capturers capture
// from. Could represent an application window or a screen. Consumers should use
// the appropriate Wgc*SourceFactory class to create wgc_capture_source objects
// of the appropriate type.
class wgc_capture_source {
public:
  explicit wgc_capture_source(desktop_capturer::source_id_t source_id);
  virtual ~wgc_capture_source();

  virtual desktop_vector get_top_left() = 0;
  // Lightweight version of IsCapturable which avoids allocating/deallocating
  // COM objects for each call. As such may return a different value than
  // IsCapturable.
  virtual bool should_be_capturable();
  virtual bool is_capturable();
  virtual bool focus_on_source();
  virtual ABI::Windows::Graphics::SizeInt32 get_size();
  HRESULT get_capture_item(
      Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> *result);
  desktop_capturer::source_id_t get_source_id() { return source_id_; }

protected:
  virtual HRESULT create_capture_item(
      Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> *result) = 0;

private:
  Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> item_;
  const desktop_capturer::source_id_t source_id_;
};

class wgc_capture_source_factory {
public:
  virtual ~wgc_capture_source_factory();

  virtual std::unique_ptr<wgc_capture_source>
      create_capture_source(desktop_capturer::source_id_t) = 0;
};

class wgc_window_source_factory final : public wgc_capture_source_factory {
public:
  wgc_window_source_factory();

  // Disallow copy and assign.
  wgc_window_source_factory(const wgc_window_source_factory &) = delete;
  wgc_window_source_factory &operator=(const wgc_window_source_factory &) = delete;

  ~wgc_window_source_factory() override;

  std::unique_ptr<wgc_capture_source> create_capture_source(desktop_capturer::source_id_t) override;
};

class wgc_screen_source_factory final : public wgc_capture_source_factory {
public:
  wgc_screen_source_factory();

  wgc_screen_source_factory(const wgc_screen_source_factory &) = delete;
  wgc_screen_source_factory &operator=(const wgc_screen_source_factory &) = delete;

  ~wgc_screen_source_factory() override;

  std::unique_ptr<wgc_capture_source> create_capture_source(desktop_capturer::source_id_t) override;
};

// Class for capturing application windows.
class wgc_window_source final : public wgc_capture_source {
public:
  explicit wgc_window_source(desktop_capturer::source_id_t source_id);

  wgc_window_source(const wgc_window_source &) = delete;
  wgc_window_source &operator=(const wgc_window_source &) = delete;

  ~wgc_window_source() override;

  desktop_vector get_top_left() override;
  ABI::Windows::Graphics::SizeInt32 get_size() override;
  bool should_be_capturable() override;
  bool is_capturable() override;
  bool focus_on_source() override;

private:
  HRESULT create_capture_item(
      Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> *result)
      override;
};

// Class for capturing screens/monitors/displays.
class wgc_screen_source final : public wgc_capture_source {
public:
  explicit wgc_screen_source(desktop_capturer::source_id_t source_id);

  wgc_screen_source(const wgc_screen_source &) = delete;
  wgc_screen_source &operator=(const wgc_screen_source &) = delete;

  ~wgc_screen_source() override;

  desktop_vector get_top_left() override;
  ABI::Windows::Graphics::SizeInt32 get_size() override;
  bool is_capturable() override;

private:
  HRESULT create_capture_item(
      Microsoft::WRL::ComPtr<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem> *result)
      override;

  // To maintain compatibility with other capturers, this class accepts a
  // device index as it's SourceId. However, WGC requires we use an HMONITOR to
  // describe which screen to capture. So, we internally convert the supplied
  // device index into an HMONITOR when `IsCapturable()` is called.
  std::optional<HMONITOR> hmonitor_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_WGC_CAPTURE_SOURCE_H_
