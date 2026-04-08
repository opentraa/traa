/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_WIN_HELP_FUNCTIONS_DS_H_
#define TRAA_BASE_DEVICES_CAMERA_WIN_HELP_FUNCTIONS_DS_H_

#include <dshow.h>

#include <atomic>
#include <type_traits>
#include <utility>

// clang-format off
DEFINE_GUID(MEDIASUBTYPE_I420,
            0x30323449,
            0x0000,
            0x0010,
            0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
DEFINE_GUID(MEDIASUBTYPE_HDYC,
            0x43594448,
            0x0000,
            0x0010,
            0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
// clang-format on

#define RELEASE_AND_CLEAR(p) \
  if (p) {                   \
    (p)->Release();          \
    (p) = NULL;              \
  }

namespace traa {
namespace base {

// Provides a reference count implementation for COM (IUnknown derived) classes.
// The implementation uses atomics for managing the ref count.
template <class T>
class com_ref_count : public T {
public:
  com_ref_count() {}

  template <class P0>
  explicit com_ref_count(P0 &&p0) : T(std::forward<P0>(p0)) {}

  STDMETHOD_(ULONG, AddRef)() override { return ++ref_count_; }

  STDMETHOD_(ULONG, Release)() override {
    ULONG count = --ref_count_;
    if (count == 0)
      delete this;
    return count;
  }

protected:
  ~com_ref_count() {}

private:
  std::atomic<ULONG> ref_count_{1};
};

// Helper functions
LONGLONG get_max_of_frame_array(LONGLONG *max_fps, long size);
IPin *get_input_pin(IBaseFilter *filter);
IPin *get_output_pin(IBaseFilter *filter, REFGUID category);
BOOL pin_matches_category(IPin *pin, REFGUID category);
void reset_media_type(AM_MEDIA_TYPE *media_type);
void free_media_type(AM_MEDIA_TYPE *media_type);
HRESULT copy_media_type(AM_MEDIA_TYPE *target, const AM_MEDIA_TYPE *source);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_WIN_HELP_FUNCTIONS_DS_H_
