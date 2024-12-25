/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/win/dxgi/dxgi_adapter_duplicator.h"

#include "base/devices/screen/win/desktop_capture_utils.h"
#include "base/log/logger.h"

#include <comdef.h>
#include <dxgi.h>

#include <algorithm>

#ifdef min
#undef min
#endif // min

#ifdef max
#undef max
#endif // max

namespace traa {
namespace base {

using Microsoft::WRL::ComPtr;

namespace {

bool is_valid_rect(const RECT &rect) { return rect.right > rect.left && rect.bottom > rect.top; }

} // namespace

dxgi_adapter_duplicator::dxgi_adapter_duplicator(const d3d_device &device) : device_(device) {}
dxgi_adapter_duplicator::dxgi_adapter_duplicator(dxgi_adapter_duplicator &&) = default;
dxgi_adapter_duplicator::~dxgi_adapter_duplicator() = default;

bool dxgi_adapter_duplicator::initialize() {
  if (do_initialize()) {
    return true;
  }
  duplicators_.clear();
  return false;
}

bool dxgi_adapter_duplicator::do_initialize() {
  for (int i = 0;; i++) {
    ComPtr<IDXGIOutput> output;
    _com_error error = device_.dxgi_adapter()->EnumOutputs(i, output.GetAddressOf());
    if (error.Error() == DXGI_ERROR_NOT_FOUND) {
      break;
    }

    if (error.Error() == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
      LOG_WARN("IDXGIAdapter::EnumOutputs returned NOT_CURRENTLY_AVAILABLE. This may happen when "
               "running in session 0.");
      break;
    }

    if (error.Error() != S_OK || !output) {
      LOG_WARN("IDXGIAdapter::EnumOutputs returned an unexpected result: {}",
               desktop_capture_utils::com_error_to_string(error));
      continue;
    }

    DXGI_OUTPUT_DESC desc;
    error = output->GetDesc(&desc);
    if (error.Error() == S_OK) {
      if (desc.AttachedToDesktop && is_valid_rect(desc.DesktopCoordinates)) {
        ComPtr<IDXGIOutput1> output1;
        error = output.As(&output1);
        if (error.Error() != S_OK || !output1) {
          LOG_WARN("Failed to convert IDXGIOutput to IDXGIOutput1, this usually means the system "
                   "does not support DirectX 11");
          continue;
        }
        dxgi_output_duplicator duplicator(device_, output1, desc);
        if (!duplicator.initialize()) {
          LOG_WARN("Failed to initialize DxgiOutputDuplicator on output {}", i);
          continue;
        }

        duplicators_.push_back(std::move(duplicator));
        desktop_rect_.union_with(duplicators_.back().get_desktop_rect());
      } else {
        LOG_ERROR("{} output {} ({}, {}) - ({}, {}) is ignored.",
                  (desc.AttachedToDesktop ? "Attached" : "Detached"), i,
                  desc.DesktopCoordinates.top, desc.DesktopCoordinates.left,
                  desc.DesktopCoordinates.bottom, desc.DesktopCoordinates.right);
      }
    } else {
      LOG_WARN("Failed to get output description of device {}", i);
    }
  }

  if (duplicators_.empty()) {
    LOG_WARN("Cannot initialize any DxgiOutputDuplicator instance.");
  }

  return !duplicators_.empty();
}

void dxgi_adapter_duplicator::setup(context_t *context) {
  context->contexts.resize(duplicators_.size());
  for (size_t i = 0; i < duplicators_.size(); i++) {
    duplicators_[i].setup(&context->contexts[i]);
  }
}

void dxgi_adapter_duplicator::unregister(const context_t *const context) {
  for (size_t i = 0; i < duplicators_.size(); i++) {
    duplicators_[i].unregister(&context->contexts[i]);
  }
}

bool dxgi_adapter_duplicator::duplicate(context_t *context, shared_desktop_frame *target) {
  for (size_t i = 0; i < duplicators_.size(); i++) {
    if (!duplicators_[i].duplicate(&context->contexts[i],
                                   duplicators_[i].get_desktop_rect().top_left(), target)) {
      return false;
    }
  }
  return true;
}

bool dxgi_adapter_duplicator::duplicate_monitor(context_t *context, int monitor_id,
                                                shared_desktop_frame *target) {
  return duplicators_[monitor_id].duplicate(&context->contexts[monitor_id], desktop_vector(),
                                            target);
}

desktop_rect dxgi_adapter_duplicator::get_screen_rect(int id) const {
  return duplicators_[id].get_desktop_rect();
}

const std::string &dxgi_adapter_duplicator::get_device_name(int id) const {
  return duplicators_[id].get_device_name();
}

int dxgi_adapter_duplicator::screen_count() const { return static_cast<int>(duplicators_.size()); }

int64_t dxgi_adapter_duplicator::get_num_frames_captured() const {
  int64_t min = INT64_MAX;
  for (const auto &duplicator : duplicators_) {
    min = std::min(min, duplicator.num_frames_captured());
  }

  return min;
}

void dxgi_adapter_duplicator::translate_rect(const desktop_vector &position) {
  desktop_rect_.translate(position);
  for (auto &duplicator : duplicators_) {
    duplicator.translate_rect(position);
  }
}

} // namespace base
} // namespace traa
