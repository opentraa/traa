/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <initguid.h> // Must come before the help_functions_ds.h include so
                      // that DEFINE_GUID() entries will be defined in this
                      // object file.

#include <cguid.h>

#include "base/devices/camera/win/help_functions_ds.h"
#include "base/logger.h"

#include <cassert>

namespace traa {
namespace base {

// This returns minimum :), which will give max frame rate...
LONGLONG get_max_of_frame_array(LONGLONG *max_fps, long size) {
  if (!max_fps || size <= 0) {
    return 0;
  }
  LONGLONG max_FPS = max_fps[0];
  for (int i = 0; i < size; i++) {
    if (max_FPS > max_fps[i])
      max_FPS = max_fps[i];
  }
  return max_FPS;
}

IPin *get_input_pin(IBaseFilter *filter) {
  IPin *pin = NULL;
  IEnumPins *pin_enum = NULL;
  filter->EnumPins(&pin_enum);
  if (pin_enum == NULL) {
    return NULL;
  }

  // get first unconnected pin
  pin_enum->Reset(); // set to first pin

  while (S_OK == pin_enum->Next(1, &pin, NULL)) {
    PIN_DIRECTION pin_dir;
    pin->QueryDirection(&pin_dir);
    if (PINDIR_INPUT == pin_dir) // This is an input pin
    {
      IPin *temp_pin = NULL;
      if (S_OK != pin->ConnectedTo(&temp_pin)) // The pin is not connected
      {
        pin_enum->Release();
        return pin;
      }
    }
    pin->Release();
  }
  pin_enum->Release();
  return NULL;
}

IPin *get_output_pin(IBaseFilter *filter, REFGUID category) {
  IPin *pin = NULL;
  IEnumPins *pin_enum = NULL;
  filter->EnumPins(&pin_enum);
  if (pin_enum == NULL) {
    return NULL;
  }
  // get first unconnected pin
  pin_enum->Reset(); // set to first pin
  while (S_OK == pin_enum->Next(1, &pin, NULL)) {
    PIN_DIRECTION pin_dir;
    pin->QueryDirection(&pin_dir);
    if (PINDIR_OUTPUT == pin_dir) // This is an output pin
    {
      if (category == GUID_NULL || pin_matches_category(pin, category)) {
        pin_enum->Release();
        return pin;
      }
    }
    pin->Release();
    pin = NULL;
  }
  pin_enum->Release();
  return NULL;
}

BOOL pin_matches_category(IPin *pin, REFGUID category) {
  BOOL found = FALSE;
  IKsPropertySet *ks = NULL;
  HRESULT hr = pin->QueryInterface(IID_PPV_ARGS(&ks));
  if (SUCCEEDED(hr)) {
    GUID pin_category;
    DWORD cb_returned;
    hr = ks->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pin_category, sizeof(GUID),
                 &cb_returned);
    if (SUCCEEDED(hr) && (cb_returned == sizeof(GUID))) {
      found = (pin_category == category);
    }
    ks->Release();
  }
  return found;
}

void reset_media_type(AM_MEDIA_TYPE *media_type) {
  if (!media_type)
    return;
  if (media_type->cbFormat != 0) {
    CoTaskMemFree(media_type->pbFormat);
    media_type->cbFormat = 0;
    media_type->pbFormat = nullptr;
  }
  if (media_type->pUnk) {
    media_type->pUnk->Release();
    media_type->pUnk = nullptr;
  }
}

void free_media_type(AM_MEDIA_TYPE *media_type) {
  if (!media_type)
    return;
  reset_media_type(media_type);
  CoTaskMemFree(media_type);
}

HRESULT copy_media_type(AM_MEDIA_TYPE *target, const AM_MEDIA_TYPE *source) {
  assert(source != target);
  *target = *source;
  if (source->cbFormat != 0) {
    assert(source->pbFormat);
    target->pbFormat = reinterpret_cast<BYTE *>(CoTaskMemAlloc(source->cbFormat));
    if (target->pbFormat == nullptr) {
      target->cbFormat = 0;
      return E_OUTOFMEMORY;
    } else {
      CopyMemory(target->pbFormat, source->pbFormat, target->cbFormat);
    }
  }

  if (target->pUnk != nullptr)
    target->pUnk->AddRef();

  return S_OK;
}

} // namespace base
} // namespace traa
