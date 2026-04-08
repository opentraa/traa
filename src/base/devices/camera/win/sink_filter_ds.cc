/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/camera/win/sink_filter_ds.h"

#include <initguid.h>
#include <dvdmedia.h> // VIDEOINFOHEADER2

#include "base/logger.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <list>
#include <vector>

// clang-format off
DEFINE_GUID(CLSID_SINKFILTER,
            0x88cdbbdc,
            0xa73b,
            0x4afa,
            0xac,
            0xbf,
            0x15,
            0xd5,
            0xe2,
            0xce,
            0x12,
            0xc3);
// clang-format on

namespace traa {
namespace base {
namespace {

// Simple enumeration implementation that enumerates over a single pin.
class enum_pins : public IEnumPins {
public:
  enum_pins(IPin *pin) : pin_(pin) {
    if (pin_)
      pin_->AddRef();
  }

protected:
  virtual ~enum_pins() {
    if (pin_)
      pin_->Release();
  }

private:
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override {
    if (riid == IID_IUnknown || riid == IID_IEnumPins) {
      *ppv = static_cast<IEnumPins *>(this);
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHOD(Clone)(IEnumPins **pins) override { return E_NOTIMPL; }

  STDMETHOD(Next)(ULONG count, IPin **pins, ULONG *fetched) override {
    assert(count > 0);
    assert(pins);
    // fetched may be NULL.

    if (pos_ > 0) {
      if (fetched)
        *fetched = 0;
      return S_FALSE;
    }

    ++pos_;
    pins[0] = pin_;
    pins[0]->AddRef();
    if (fetched)
      *fetched = 1;

    return count == 1 ? S_OK : S_FALSE;
  }

  STDMETHOD(Skip)(ULONG count) override { return E_NOTIMPL; }

  STDMETHOD(Reset)() override {
    pos_ = 0;
    return S_OK;
  }

  IPin *pin_;
  int pos_ = 0;
};

bool is_media_type_partial_match(const AM_MEDIA_TYPE &a, const AM_MEDIA_TYPE &b) {
  if (b.majortype != GUID_NULL && a.majortype != b.majortype)
    return false;

  if (b.subtype != GUID_NULL && a.subtype != b.subtype)
    return false;

  if (b.formattype != GUID_NULL) {
    // if the format block is specified then it must match exactly
    if (a.formattype != b.formattype)
      return false;

    if (a.cbFormat != b.cbFormat)
      return false;

    if (a.cbFormat != 0 && memcmp(a.pbFormat, b.pbFormat, a.cbFormat) != 0)
      return false;
  }

  return true;
}

bool is_media_type_fully_specified(const AM_MEDIA_TYPE &type) {
  return type.majortype != GUID_NULL && type.formattype != GUID_NULL;
}

BYTE *alloc_media_type_format_buffer(AM_MEDIA_TYPE *media_type, ULONG length) {
  assert(length);
  if (media_type->cbFormat == length)
    return media_type->pbFormat;

  BYTE *buffer = static_cast<BYTE *>(CoTaskMemAlloc(length));
  if (!buffer)
    return nullptr;

  if (media_type->pbFormat) {
    assert(media_type->cbFormat);
    CoTaskMemFree(media_type->pbFormat);
    media_type->pbFormat = nullptr;
  }

  media_type->cbFormat = length;
  media_type->pbFormat = buffer;
  return buffer;
}

void get_sample_properties(IMediaSample *sample, AM_SAMPLE2_PROPERTIES *props) {
  IMediaSample2 *sample2 = nullptr;
  if (SUCCEEDED(sample->QueryInterface(IID_IMediaSample2, reinterpret_cast<void **>(&sample2)))) {
    sample2->GetProperties(sizeof(*props), reinterpret_cast<BYTE *>(props));
    sample2->Release();
    return;
  }

  //  Get the properties the hard way.
  props->cbData = sizeof(*props);
  props->dwTypeSpecificFlags = 0;
  props->dwStreamId = AM_STREAM_MEDIA;
  props->dwSampleFlags = 0;

  if (sample->IsDiscontinuity() == S_OK)
    props->dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;

  if (sample->IsPreroll() == S_OK)
    props->dwSampleFlags |= AM_SAMPLE_PREROLL;

  if (sample->IsSyncPoint() == S_OK)
    props->dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;

  if (SUCCEEDED(sample->GetTime(&props->tStart, &props->tStop)))
    props->dwSampleFlags |= AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID;

  if (sample->GetMediaType(&props->pMediaType) == S_OK)
    props->dwSampleFlags |= AM_SAMPLE_TYPECHANGED;

  sample->GetPointer(&props->pbBuffer);
  props->lActual = sample->GetActualDataLength();
  props->cbBuffer = sample->GetSize();
}

class media_types_enum : public IEnumMediaTypes {
public:
  media_types_enum(const video_capture_capability &capability)
      : capability_(capability),
        format_preference_order_(
            {// Default preferences, sorted by cost-to-convert-to-i420.
             video_type::k_i420, video_type::k_yuy2, video_type::k_rgb24, video_type::k_uyvy,
             video_type::k_mjpeg}) {
    // Use the preferred video type, if supported.
    auto it = std::find(format_preference_order_.begin(), format_preference_order_.end(),
                        capability_.video_type);
    if (it != format_preference_order_.end()) {
      LOG_INFO("selected video type: {}", static_cast<int>(capability_.video_type));
      // Move it to the front of the list, if it isn't already there.
      if (it != format_preference_order_.begin()) {
        format_preference_order_.splice(format_preference_order_.begin(),
                                        format_preference_order_, it, std::next(it));
      }
    } else {
      LOG_WARN("unsupported video type: {}, using default preference list",
               static_cast<int>(capability_.video_type));
    }
  }

protected:
  virtual ~media_types_enum() {}

private:
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override {
    if (riid == IID_IUnknown || riid == IID_IEnumMediaTypes) {
      *ppv = static_cast<IEnumMediaTypes *>(this);
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  // IEnumMediaTypes
  STDMETHOD(Clone)(IEnumMediaTypes **pins) override { return E_NOTIMPL; }

  STDMETHOD(Next)(ULONG count, AM_MEDIA_TYPE **types, ULONG *fetched) override {
    assert(count > 0);
    assert(types);
    // fetched may be NULL.
    if (fetched)
      *fetched = 0;

    for (ULONG i = 0;
         i < count && pos_ < static_cast<int>(format_preference_order_.size()); ++i) {
      AM_MEDIA_TYPE *media_type =
          reinterpret_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
      ZeroMemory(media_type, sizeof(*media_type));
      types[i] = media_type;
      VIDEOINFOHEADER *vih = reinterpret_cast<VIDEOINFOHEADER *>(
          alloc_media_type_format_buffer(media_type, sizeof(VIDEOINFOHEADER)));
      ZeroMemory(vih, sizeof(*vih));
      vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      vih->bmiHeader.biPlanes = 1;
      vih->bmiHeader.biClrImportant = 0;
      vih->bmiHeader.biClrUsed = 0;
      if (capability_.max_fps != 0)
        vih->AvgTimePerFrame = 10000000 / capability_.max_fps;

      SetRectEmpty(&vih->rcSource);
      SetRectEmpty(&vih->rcTarget);

      media_type->majortype = MEDIATYPE_Video;
      media_type->formattype = FORMAT_VideoInfo;
      media_type->bTemporalCompression = FALSE;

      // Set format information.
      auto format_it = std::next(format_preference_order_.begin(), pos_++);
      set_media_info_from_video_type(*format_it, &vih->bmiHeader, media_type);

      vih->bmiHeader.biWidth = capability_.width;
      vih->bmiHeader.biHeight = capability_.height;
      vih->bmiHeader.biSizeImage =
          ((vih->bmiHeader.biBitCount / 4) * capability_.height * capability_.width) / 2;

      assert(vih->bmiHeader.biSizeImage);
      media_type->lSampleSize = vih->bmiHeader.biSizeImage;
      media_type->bFixedSizeSamples = true;
      if (fetched)
        ++(*fetched);
    }
    return pos_ == static_cast<int>(format_preference_order_.size()) ? S_FALSE : S_OK;
  }

  static void set_media_info_from_video_type(video_type vtype, BITMAPINFOHEADER *bitmap_header,
                                             AM_MEDIA_TYPE *media_type) {
    switch (vtype) {
    case video_type::k_i420:
      bitmap_header->biCompression = MAKEFOURCC('I', '4', '2', '0');
      bitmap_header->biBitCount = 12;
      media_type->subtype = MEDIASUBTYPE_I420;
      break;
    case video_type::k_yuy2:
      bitmap_header->biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
      bitmap_header->biBitCount = 16;
      media_type->subtype = MEDIASUBTYPE_YUY2;
      break;
    case video_type::k_rgb24:
      bitmap_header->biCompression = BI_RGB;
      bitmap_header->biBitCount = 24;
      media_type->subtype = MEDIASUBTYPE_RGB24;
      break;
    case video_type::k_uyvy:
      bitmap_header->biCompression = MAKEFOURCC('U', 'Y', 'V', 'Y');
      bitmap_header->biBitCount = 16;
      media_type->subtype = MEDIASUBTYPE_UYVY;
      break;
    case video_type::k_mjpeg:
      bitmap_header->biCompression = MAKEFOURCC('M', 'J', 'P', 'G');
      bitmap_header->biBitCount = 12;
      media_type->subtype = MEDIASUBTYPE_MJPG;
      break;
    default:
      assert(false && "unexpected video type");
    }
  }

  STDMETHOD(Skip)(ULONG count) override { return E_NOTIMPL; }

  STDMETHOD(Reset)() override {
    pos_ = 0;
    return S_OK;
  }

  int pos_ = 0;
  const video_capture_capability capability_;
  std::list<video_type> format_preference_order_;
};

} // namespace

// ---------------------------------------------------------------------------
// translate_media_type_to_video_capture_capability
// ---------------------------------------------------------------------------

bool translate_media_type_to_video_capture_capability(const AM_MEDIA_TYPE *media_type,
                                                      video_capture_capability *capability) {
  assert(capability);
  if (!media_type || media_type->majortype != MEDIATYPE_Video || !media_type->pbFormat) {
    return false;
  }

  const BITMAPINFOHEADER *bih = nullptr;
  if (media_type->formattype == FORMAT_VideoInfo) {
    bih = &reinterpret_cast<VIDEOINFOHEADER *>(media_type->pbFormat)->bmiHeader;
  } else if (media_type->formattype != FORMAT_VideoInfo2) {
    bih = &reinterpret_cast<VIDEOINFOHEADER2 *>(media_type->pbFormat)->bmiHeader;
  } else {
    return false;
  }

  LOG_INFO("translate_media_type_to_video_capture_capability width:{} height:{} compression:0x{:x}",
           bih->biWidth, bih->biHeight, static_cast<unsigned>(bih->biCompression));

  const GUID &sub_type = media_type->subtype;
  if (sub_type == MEDIASUBTYPE_MJPG &&
      bih->biCompression == MAKEFOURCC('M', 'J', 'P', 'G')) {
    capability->video_type = video_type::k_mjpeg;
  } else if (sub_type == MEDIASUBTYPE_I420 &&
             bih->biCompression == MAKEFOURCC('I', '4', '2', '0')) {
    capability->video_type = video_type::k_i420;
  } else if (sub_type == MEDIASUBTYPE_YUY2 &&
             bih->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2')) {
    capability->video_type = video_type::k_yuy2;
  } else if (sub_type == MEDIASUBTYPE_UYVY &&
             bih->biCompression == MAKEFOURCC('U', 'Y', 'V', 'Y')) {
    capability->video_type = video_type::k_uyvy;
  } else if (sub_type == MEDIASUBTYPE_HDYC) {
    capability->video_type = video_type::k_uyvy;
  } else if (sub_type == MEDIASUBTYPE_RGB24 && bih->biCompression == BI_RGB) {
    capability->video_type = video_type::k_rgb24;
  } else {
    return false;
  }

  // Store the incoming width and height.
  capability->width = bih->biWidth;

  // Store the incoming height,
  // for RGB24 we assume the frame to be upside down.
  if (sub_type == MEDIASUBTYPE_RGB24 && bih->biHeight > 0) {
    capability->height = -(bih->biHeight);
  } else {
    capability->height = abs(bih->biHeight);
  }

  return true;
}

// ---------------------------------------------------------------------------
// capture_input_pin
// ---------------------------------------------------------------------------

capture_input_pin::capture_input_pin(capture_sink_filter *filter) {
  // No reference held to avoid circular references.
  info_.pFilter = filter;
  info_.dir = PINDIR_INPUT;
}

capture_input_pin::~capture_input_pin() {
  reset_media_type(&media_type_);
}

HRESULT capture_input_pin::set_requested_capability(const video_capture_capability &capability) {
  assert(filter()->is_stopped());
  requested_capability_ = capability;
  resulting_capability_ = video_capture_capability();
  return S_OK;
}

void capture_input_pin::on_filter_activated() {
  runtime_error_ = false;
  flushing_ = false;
  capture_thread_id_ = 0;
}

void capture_input_pin::on_filter_deactivated() {
  // Expedite shutdown by raising the flushing flag so no further processing
  // on the capture thread occurs.
  flushing_ = true;
  if (allocator_)
    allocator_->Decommit();
}

capture_sink_filter *capture_input_pin::filter() const {
  return static_cast<capture_sink_filter *>(info_.pFilter);
}

HRESULT capture_input_pin::attempt_connection(IPin *receive_pin,
                                              const AM_MEDIA_TYPE *media_type) {
  assert(filter()->is_stopped());

  // Check that the connection is valid.
  HRESULT hr = check_direction(receive_pin);
  if (FAILED(hr))
    return hr;

  if (!translate_media_type_to_video_capture_capability(media_type, &resulting_capability_)) {
    clear_allocator(true);
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  // See if the other pin will accept this type.
  hr = receive_pin->ReceiveConnection(static_cast<IPin *>(this), media_type);
  if (FAILED(hr)) {
    receive_pin_ = nullptr;
    return hr;
  }

  // Should have been set as part of the connect process.
  assert(receive_pin_ == receive_pin);

  reset_media_type(&media_type_);
  copy_media_type(&media_type_, media_type);

  return S_OK;
}

std::vector<AM_MEDIA_TYPE *>
capture_input_pin::determine_candidate_formats(IPin *receive_pin,
                                               const AM_MEDIA_TYPE *media_type) {
  assert(receive_pin);
  assert(media_type);

  std::vector<AM_MEDIA_TYPE *> ret;

  for (int i = 0; i < 2; i++) {
    IEnumMediaTypes *types = nullptr;
    if (i == 0) {
      // First time around, try types from receive_pin.
      receive_pin->EnumMediaTypes(&types);
    } else {
      // Then try ours.
      EnumMediaTypes(&types);
    }

    if (types) {
      while (true) {
        ULONG fetched = 0;
        AM_MEDIA_TYPE *this_type = nullptr;
        if (types->Next(1, &this_type, &fetched) != S_OK)
          break;

        if (is_media_type_partial_match(*this_type, *media_type)) {
          ret.push_back(this_type);
        } else {
          free_media_type(this_type);
        }
      }
      types->Release();
    }
  }

  return ret;
}

void capture_input_pin::clear_allocator(bool decommit) {
  if (!allocator_)
    return;
  if (decommit)
    allocator_->Decommit();
  allocator_->Release();
  allocator_ = nullptr;
}

HRESULT capture_input_pin::check_direction(IPin *pin) const {
  PIN_DIRECTION pd;
  pin->QueryDirection(&pd);
  return pd == info_.dir ? VFW_E_INVALID_DIRECTION : S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::QueryInterface(REFIID riid, void **ppv) {
  (*ppv) = nullptr;
  if (riid == IID_IUnknown || riid == IID_IMemInputPin) {
    *ppv = static_cast<IMemInputPin *>(this);
  } else if (riid == IID_IPin) {
    *ppv = static_cast<IPin *>(this);
  }

  if (!(*ppv))
    return E_NOINTERFACE;

  static_cast<IMemInputPin *>(this)->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::Connect(IPin *receive_pin,
                                                             const AM_MEDIA_TYPE *media_type) {
  if (!media_type || !receive_pin)
    return E_POINTER;

  if (!filter()->is_stopped())
    return VFW_E_NOT_STOPPED;

  if (receive_pin_) {
    return VFW_E_ALREADY_CONNECTED;
  }

  if (is_media_type_fully_specified(*media_type))
    return attempt_connection(receive_pin, media_type);

  auto types = determine_candidate_formats(receive_pin, media_type);
  bool connected = false;
  for (auto *type : types) {
    if (!connected && attempt_connection(receive_pin, media_type) == S_OK)
      connected = true;

    free_media_type(type);
  }

  return connected ? S_OK : VFW_E_NO_ACCEPTABLE_TYPES;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::ReceiveConnection(IPin *connector, const AM_MEDIA_TYPE *media_type) {
  assert(filter()->is_stopped());

  if (receive_pin_) {
    return VFW_E_ALREADY_CONNECTED;
  }

  HRESULT hr = check_direction(connector);
  if (FAILED(hr))
    return hr;

  if (!translate_media_type_to_video_capture_capability(media_type, &resulting_capability_))
    return VFW_E_TYPE_NOT_ACCEPTED;

  // Complete the connection.
  receive_pin_ = connector;
  receive_pin_->AddRef();
  reset_media_type(&media_type_);
  copy_media_type(&media_type_, media_type);

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::Disconnect() {
  if (!filter()->is_stopped())
    return VFW_E_NOT_STOPPED;

  if (!receive_pin_)
    return S_FALSE;

  clear_allocator(true);
  receive_pin_->Release();
  receive_pin_ = nullptr;

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::ConnectedTo(IPin **pin) {
  if (!receive_pin_)
    return VFW_E_NOT_CONNECTED;

  *pin = receive_pin_;
  receive_pin_->AddRef();

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::ConnectionMediaType(AM_MEDIA_TYPE *media_type) {
  if (!receive_pin_)
    return VFW_E_NOT_CONNECTED;

  copy_media_type(media_type, &media_type_);

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::QueryPinInfo(PIN_INFO *info) {
  *info = info_;
  if (info_.pFilter)
    info_.pFilter->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::QueryDirection(PIN_DIRECTION *pin_dir) {
  *pin_dir = info_.dir;
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::QueryId(LPWSTR *id) {
  size_t len = lstrlenW(info_.achName);
  *id = reinterpret_cast<LPWSTR>(CoTaskMemAlloc((len + 1) * sizeof(wchar_t)));
  lstrcpyW(*id, info_.achName);
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::QueryAccept(const AM_MEDIA_TYPE *media_type) {
  assert(filter()->is_stopped());
  video_capture_capability capability(resulting_capability_);
  return translate_media_type_to_video_capture_capability(media_type, &capability) ? S_FALSE : S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::EnumMediaTypes(IEnumMediaTypes **types) {
  *types = new com_ref_count<media_types_enum>(requested_capability_);
  (*types)->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::QueryInternalConnections(IPin **pins, ULONG *count) {
  return E_NOTIMPL;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::EndOfStream() { return S_OK; }

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::BeginFlush() {
  flushing_ = true;
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::EndFlush() {
  flushing_ = false;
  runtime_error_ = false;
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::NewSegment(REFERENCE_TIME start,
                                                                REFERENCE_TIME stop, double rate) {
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::GetAllocator(IMemAllocator **allocator) {
  if (allocator_ == nullptr) {
    HRESULT hr = CoCreateInstance(CLSID_MemoryAllocator, 0, CLSCTX_INPROC_SERVER,
                                  IID_IMemAllocator, reinterpret_cast<void **>(&allocator_));
    if (FAILED(hr))
      return hr;
  }
  *allocator = allocator_;
  allocator_->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::NotifyAllocator(IMemAllocator *allocator, BOOL read_only) {
  // Swap the allocator, managing ref counts manually.
  IMemAllocator *old = allocator_;
  allocator_ = allocator;
  if (allocator_)
    allocator_->AddRef();
  if (old)
    old->Release();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP
capture_input_pin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *props) {
  return E_NOTIMPL;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::Receive(IMediaSample *media_sample) {
  capture_sink_filter *const f = filter();

  if (flushing_.load(std::memory_order_relaxed))
    return S_FALSE;

  if (runtime_error_.load(std::memory_order_relaxed))
    return VFW_E_RUNTIME_ERROR;

  if (!capture_thread_id_) {
    // Make sure we set the thread name only once.
    capture_thread_id_ = GetCurrentThreadId();
  }

  AM_SAMPLE2_PROPERTIES sample_props = {};
  get_sample_properties(media_sample, &sample_props);
  // Has the format changed in this sample?
  if (sample_props.dwSampleFlags & AM_SAMPLE_TYPECHANGED) {
    // Note: This will modify resulting_capability_.
    if (!translate_media_type_to_video_capture_capability(sample_props.pMediaType,
                                                         &resulting_capability_)) {
      // Raise a runtime error if we fail the media type.
      runtime_error_ = true;
      EndOfStream();
      f->notify_event(EC_ERRORABORT, VFW_E_TYPE_NOT_ACCEPTED, 0);
      return VFW_E_INVALIDMEDIATYPE;
    }
  }

  f->process_captured_frame(sample_props.pbBuffer, sample_props.lActual, resulting_capability_);

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::ReceiveMultiple(IMediaSample **samples,
                                                                     long count,
                                                                     long *processed) {
  HRESULT hr = S_OK;
  *processed = 0;
  while (count-- > 0) {
    hr = Receive(samples[*processed]);
    if (hr != S_OK)
      break;
    ++(*processed);
  }
  return hr;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_input_pin::ReceiveCanBlock() { return S_FALSE; }

// ---------------------------------------------------------------------------
// capture_sink_filter
// ---------------------------------------------------------------------------

capture_sink_filter::capture_sink_filter(video_capture_impl *capture_observer)
    : input_pin_(new com_ref_count<capture_input_pin>(this)),
      capture_observer_(capture_observer) {}

capture_sink_filter::~capture_sink_filter() {}

HRESULT
capture_sink_filter::set_requested_capability(const video_capture_capability &capability) {
  return input_pin_->set_requested_capability(capability);
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::GetState(DWORD msecs,
                                                                FILTER_STATE *state) {
  *state = state_;
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::SetSyncSource(IReferenceClock *clock) {
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::GetSyncSource(IReferenceClock **clock) {
  return E_NOTIMPL;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::Pause() {
  state_ = State_Paused;
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::Run(REFERENCE_TIME tStart) {
  if (state_ == State_Stopped)
    Pause();

  state_ = State_Running;
  input_pin_->on_filter_activated();

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::Stop() {
  if (state_ == State_Stopped)
    return S_OK;

  state_ = State_Stopped;
  input_pin_->on_filter_deactivated();

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::EnumPins(IEnumPins **pins) {
  *pins = new com_ref_count<enum_pins>(input_pin_);
  (*pins)->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::FindPin(LPCWSTR id, IPin **pin) {
  return VFW_E_NOT_FOUND;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::QueryFilterInfo(FILTER_INFO *info) {
  *info = info_;
  if (info->pGraph)
    info->pGraph->AddRef();
  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::JoinFilterGraph(IFilterGraph *graph,
                                                                       LPCWSTR name) {
  assert(is_stopped());

  // Note, since a reference to the filter is held by the graph manager,
  // filters must not hold a reference to the graph.
  info_.pGraph = graph; // No AddRef().
  sink_ = nullptr;

  if (info_.pGraph) {
    // Use direct QueryInterface instead of GetComInterface.
    IMediaEventSink *sink = nullptr;
    if (SUCCEEDED(info_.pGraph->QueryInterface(IID_IMediaEventSink,
                                               reinterpret_cast<void **>(&sink)))) {
      sink_ = sink;
      // Release the reference since we don't want to hold one.
      sink->Release();
    }
  }

  info_.achName[0] = L'\0';
  if (name)
    lstrcpynW(info_.achName, name, static_cast<int>(std::size(info_.achName)));

  return S_OK;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::QueryVendorInfo(LPWSTR *vendor_info) {
  return E_NOTIMPL;
}

void capture_sink_filter::process_captured_frame(unsigned char *buffer, size_t length,
                                                 const video_capture_capability &frame_info) {
  // Called on the capture thread.
  capture_observer_->incoming_frame(buffer, length, frame_info);
}

void capture_sink_filter::notify_event(long code, LONG_PTR param1, LONG_PTR param2) {
  // Called on the capture thread.
  if (!sink_)
    return;

  if (EC_COMPLETE == code)
    param2 = reinterpret_cast<LONG_PTR>(static_cast<IBaseFilter *>(this));
  sink_->Notify(code, param1, param2);
}

bool capture_sink_filter::is_stopped() const { return state_ == State_Stopped; }

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::QueryInterface(REFIID riid, void **ppv) {
  if (riid == IID_IUnknown || riid == IID_IPersist || riid == IID_IBaseFilter) {
    *ppv = static_cast<IBaseFilter *>(this);
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP capture_sink_filter::GetClassID(CLSID *clsid) {
  *clsid = CLSID_SINKFILTER;
  return S_OK;
}

} // namespace base
} // namespace traa
