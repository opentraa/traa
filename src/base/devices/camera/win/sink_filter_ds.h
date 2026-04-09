/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_CAMERA_WIN_SINK_FILTER_DS_H_
#define TRAA_BASE_DEVICES_CAMERA_WIN_SINK_FILTER_DS_H_

#include <dshow.h>

#include <atomic>
#include <memory>
#include <vector>

#include "base/devices/camera/video_capture_impl.h"
#include "base/devices/camera/win/help_functions_ds.h"

namespace traa {
namespace base {

// Forward declaration.
class capture_sink_filter;

// Input pin for camera input.
// Implements IMemInputPin and IPin.
class capture_input_pin : public IMemInputPin, public IPin {
public:
  capture_input_pin(capture_sink_filter *filter);

  HRESULT set_requested_capability(const video_capture_capability &capability);

  // Notifications from the filter.
  void on_filter_activated();
  void on_filter_deactivated();

protected:
  virtual ~capture_input_pin();

private:
  capture_sink_filter *filter() const;

  HRESULT attempt_connection(IPin *receive_pin, const AM_MEDIA_TYPE *media_type);
  std::vector<AM_MEDIA_TYPE *> determine_candidate_formats(IPin *receive_pin,
                                                           const AM_MEDIA_TYPE *media_type);
  void clear_allocator(bool decommit);
  HRESULT check_direction(IPin *pin) const;

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override;

  // IPin
  STDMETHOD(Connect)(IPin *receive_pin, const AM_MEDIA_TYPE *media_type) override;
  STDMETHOD(ReceiveConnection)(IPin *connector, const AM_MEDIA_TYPE *media_type) override;
  STDMETHOD(Disconnect)() override;
  STDMETHOD(ConnectedTo)(IPin **pin) override;
  STDMETHOD(ConnectionMediaType)(AM_MEDIA_TYPE *media_type) override;
  STDMETHOD(QueryPinInfo)(PIN_INFO *info) override;
  STDMETHOD(QueryDirection)(PIN_DIRECTION *pin_dir) override;
  STDMETHOD(QueryId)(LPWSTR *id) override;
  STDMETHOD(QueryAccept)(const AM_MEDIA_TYPE *media_type) override;
  STDMETHOD(EnumMediaTypes)(IEnumMediaTypes **types) override;
  STDMETHOD(QueryInternalConnections)(IPin **pins, ULONG *count) override;
  STDMETHOD(EndOfStream)() override;
  STDMETHOD(BeginFlush)() override;
  STDMETHOD(EndFlush)() override;
  STDMETHOD(NewSegment)(REFERENCE_TIME start, REFERENCE_TIME stop, double rate) override;

  // IMemInputPin
  STDMETHOD(GetAllocator)(IMemAllocator **allocator) override;
  STDMETHOD(NotifyAllocator)(IMemAllocator *allocator, BOOL read_only) override;
  STDMETHOD(GetAllocatorRequirements)(ALLOCATOR_PROPERTIES *props) override;
  STDMETHOD(Receive)(IMediaSample *sample) override;
  STDMETHOD(ReceiveMultiple)(IMediaSample **samples, long count, long *processed) override;
  STDMETHOD(ReceiveCanBlock)() override;

  video_capture_capability requested_capability_;
  // Accessed on the main thread when filter()->is_stopped() (capture thread not
  // running), otherwise accessed on the capture thread.
  video_capture_capability resulting_capability_;
  DWORD capture_thread_id_ = 0;
  IMemAllocator *allocator_ = nullptr;
  IPin *receive_pin_ = nullptr;
  std::atomic_bool flushing_{false};
  std::atomic_bool runtime_error_{false};
  // Holds a referenceless pointer to the owning filter, the name and
  // direction of the pin. The filter pointer can be considered const.
  PIN_INFO info_ = {};
  AM_MEDIA_TYPE media_type_ = {};
};

// Implement IBaseFilter (including IPersist and IMediaFilter).
class capture_sink_filter : public IBaseFilter {
public:
  capture_sink_filter(video_capture_impl *capture_observer);

  HRESULT set_requested_capability(const video_capture_capability &capability);

  // Called on the capture thread.
  void process_captured_frame(unsigned char *buffer, size_t length,
                              const video_capture_capability &frame_info);

  void notify_event(long code, LONG_PTR param1, LONG_PTR param2);
  bool is_stopped() const;

  // IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override;

  // IPersist
  STDMETHOD(GetClassID)(CLSID *clsid) override;

  // IMediaFilter
  STDMETHOD(GetState)(DWORD msecs, FILTER_STATE *state) override;
  STDMETHOD(SetSyncSource)(IReferenceClock *clock) override;
  STDMETHOD(GetSyncSource)(IReferenceClock **clock) override;
  STDMETHOD(Pause)() override;
  STDMETHOD(Run)(REFERENCE_TIME start) override;
  STDMETHOD(Stop)() override;

  // IBaseFilter
  STDMETHOD(EnumPins)(IEnumPins **pins) override;
  STDMETHOD(FindPin)(LPCWSTR id, IPin **pin) override;
  STDMETHOD(QueryFilterInfo)(FILTER_INFO *info) override;
  STDMETHOD(JoinFilterGraph)(IFilterGraph *graph, LPCWSTR name) override;
  STDMETHOD(QueryVendorInfo)(LPWSTR *vendor_info) override;

protected:
  virtual ~capture_sink_filter();

private:
  com_ref_count<capture_input_pin> *input_pin_;
  video_capture_impl *const capture_observer_;
  FILTER_INFO info_ = {};
  // Set/cleared in JoinFilterGraph. The filter must be stopped (no capture)
  // at that time, so no lock is required. While the state is not stopped,
  // the sink will be used from the capture thread.
  IMediaEventSink *sink_ = nullptr;
  FILTER_STATE state_ = State_Stopped;
};

// Returns true if the media type is supported, false otherwise.
// For supported types, the `capability` will be populated accordingly.
bool translate_media_type_to_video_capture_capability(const AM_MEDIA_TYPE *media_type,
                                                      video_capture_capability *capability);

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_CAMERA_WIN_SINK_FILTER_DS_H_
