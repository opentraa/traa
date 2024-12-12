/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_DUPLICATOR_CONTROLLER_H_
#define TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_DUPLICATOR_CONTROLLER_H_

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/shared_desktop_frame.h"
#include "base/devices/screen/win/d3d_device.h"
#include "base/devices/screen/win/display_configuration_monitor.h"
#include "base/devices/screen/win/dxgi/dxgi_adapter_duplicator.h"
#include "base/devices/screen/win/dxgi/dxgi_context.h"
#include "base/devices/screen/win/dxgi/dxgi_frame.h"
#include "base/thread_annotations.h"

#include <d3dcommon.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace traa {
namespace base {

// A controller for all the objects we need to call Windows DirectX capture APIs
// It's a singleton because only one IDXGIOutputDuplication instance per monitor
// is allowed per application.
//
// Consumers should create a dxgi_duplicator_controller::context_t and keep it
// throughout their lifetime, and pass it when calling duplicate(). Consumers
// can also call is_supported() to determine whether the system supports DXGI
// duplicator or not. If a previous is_supported() function call returns true,
// but a later duplicate() returns false, this usually means the display mode is
// changing. Consumers should retry after a while. (Typically 50 milliseconds,
// but according to hardware performance, this time may vary.)
// The underlying dxgi_output_duplicator may take an additional reference on the
// frame passed in to the duplicate methods so that they can guarantee delivery
// of new frames when requested; since if there have been no updates to the
// surface, they may be unable to capture a frame.
class dxgi_duplicator_controller {
public:
  using context_t = dxgi_frame_context;

  // A collection of D3d information we are interested in, which may impact
  // capturer performance or reliability.
  struct d3d_info {
    // Each video adapter has its own D3D_FEATURE_LEVEL, so this structure
    // contains the minimum and maximium D3D_FEATURE_LEVELs current system
    // supports.
    // Both fields can be 0, which is the default value to indicate no valid
    // D3D_FEATURE_LEVEL has been retrieved from underlying OS APIs.
    D3D_FEATURE_LEVEL min_feature_level;
    D3D_FEATURE_LEVEL max_feature_level;

    // TODO(zijiehe): Add more fields, such as manufacturer name, mode, driver
    // version.
  };

  // These values are persisted to logs. Entries should not be renumbered or
  // reordered and numeric values should never be reused. This enum corresponds
  // to WebRtcDirectXCapturerResult in tools/metrics/histograms/enums.xml.
  enum class duplicate_result {
    succeeded = 0,
    unsupported_session = 1,
    frame_prepare_failed = 2,
    initialization_failed = 3,
    duplication_failed = 4,
    invalid_monitor_id = 5,
    max_value = invalid_monitor_id
  };

  // Converts `result` into user-friendly string representation. The return
  // value should not be used to identify error types.
  static std::string result_name(duplicate_result result);

  // Returns the singleton instance of dxgi_duplicator_controller.
  static std::shared_ptr<dxgi_duplicator_controller> instance();

  // See screen_capturer_win_directx::is_current_session_supported().
  static bool is_current_session_supported();

  // All the following public functions implicitly call initialize() function.

  // Detects whether the system supports DXGI based capturer.
  bool is_supported();

  // Returns a copy of d3d_info composed by last initialize() function call. This
  // function always copies the latest information into `info`. But once the
  // function returns false, the information in `info` may not accurate.
  bool retrieve_d3d_info(d3d_info *info);

  // Captures current screen and writes into `frame`. May retain a reference to
  // `frame`'s underlying |shared_desktop_frame|.
  // TODO(zijiehe): Windows cannot guarantee the frames returned by each
  // IDXGIOutputDuplication are synchronized. But we are using a totally
  // different threading model than the way Windows suggested, it's hard to
  // synchronize them manually. We should find a way to do it.
  duplicate_result duplicate(dxgi_frame *frame);

  // Captures one monitor and writes into target. `monitor_id` must be >= 0. If
  // `monitor_id` is greater than the total screen count of all the Duplicators,
  // this function returns false. May retain a reference to `frame`'s underlying
  // |shared_desktop_frame|.
  duplicate_result duplicate_monitor(dxgi_frame *frame, int monitor_id);

  // Returns dpi of current system. Returns an empty desktop_vector if system
  // does not support DXGI based capturer.
  desktop_vector system_dpi();

  // Returns the count of screens on the system. These screens can be retrieved
  // by an integer in the range of [0, screen_count()). If system does not
  // support DXGI based capturer, this function returns 0.
  int screen_count();

  // Returns the device names of all screens on the system in utf8 encoding.
  // These screens can be retrieved by an integer in the range of
  // [0, output->size()). If system does not support DXGI based capturer, this
  // function returns false.
  bool get_device_names(std::vector<std::string> *output);

private:
  // dxgi_frame_context calls private unregister(context_t*) function in reset().
  friend void dxgi_frame_context::reset();

  // std::shared_ptr<dxgi_duplicator_controller> accesses private constructor() and
  // release() functions.
  friend class std::shared_ptr<dxgi_duplicator_controller>;

  // A private constructor to ensure consumers to use
  // dxgi_duplicator_controller::instance().
  dxgi_duplicator_controller();

  // Not implemented: The singleton dxgi_duplicator_controller instance should not
  // be deleted.
  ~dxgi_duplicator_controller();

  // Does the real duplication work. Setting `monitor_id` < 0 to capture entire
  // screen. This function calls initialize(). And if the duplication failed,
  // this function calls deinitialize() to ensure the Dxgi components can be
  // reinitialized next time.
  duplicate_result do_duplicate(dxgi_frame *frame, int monitor_id);

  // Unload all the DXGI components and releases the resources. This function
  // wraps deinitialize() with `mutex_`.
  void unload();

  // Unregisters context_t from this instance and all DxgiAdapterDuplicator(s)
  // it owns.
  void unregister(const context_t *const context);

  // All functions below should be called in `mutex_` locked scope and should be
  // after a successful initialize().

  // If current instance has not been initialized, executes do_initialize()
  // function, and returns initialize result. Otherwise directly returns true.
  // This function may calls deinitialize() if initialization failed.
  bool initialize() TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Does the real initialization work, this function should only be called in
  // initialize().
  bool do_initialize() TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Clears all COM components referred to by this instance. So next duplicate()
  // call will eventually initialize this instance again.
  void deinitialize() TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // A helper function to check whether a context_t has been expired.
  bool context_expired(const context_t *const context) const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Updates context_t if needed.
  void setup(context_t *context) TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  bool do_duplicate_unlocked(context_t *context, int monitor_id, shared_desktop_frame *target)
      TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Captures all monitors.
  bool do_duplicate_all(context_t *context, shared_desktop_frame *target)
      TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Captures one monitor.
  bool do_duplicate_one(context_t *context, int monitor_id, shared_desktop_frame *target)
      TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // The minimum get_num_frames_captured() returned by `duplicators_`.
  int64_t get_num_frames_captured() const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Returns a desktop_size to cover entire `desktop_rect_`.
  desktop_size get_desktop_size() const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Returns the size of one screen. `id` should be >= 0. If system does not
  // support DXGI based capturer, or `id` is greater than the total screen count
  // of all the Duplicators, this function returns an empty desktop_rect.
  desktop_rect get_screen_rect(int id) const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  int screen_count_unlocked() const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  void get_device_names_unlocked(std::vector<std::string> *output) const
      TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Returns the desktop size of the selected screen `monitor_id`. Setting
  // `monitor_id` < 0 to return the entire screen size.
  desktop_size selected_desktop_size(int monitor_id) const TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Retries do_duplicate_all() for several times until get_num_frames_captured() is
  // large enough. Returns false if do_duplicate_all() returns false, or
  // get_num_frames_captured() has never reached the requirement.
  // According to http://crbug.com/682112, dxgi capturer returns a black frame
  // during first several capture attempts.
  bool ensure_frame_captured(context_t *context, shared_desktop_frame *target)
      TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Moves `desktop_rect_` and all underlying `duplicators_`, putting top left
  // corner of the desktop at (0, 0). This is necessary because DXGI_OUTPUT_DESC
  // may return negative coordinates. Called from DoInitialize() after all
  // DxgiAdapterDuplicator and DxgiOutputDuplicator instances are initialized.
  void translate_rect() TRAA_EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // The count of references which are now "living".
  std::atomic_int refcount_;

  // This lock must be locked whenever accessing any of the following objects.
  std::mutex mutex_;

  // A self-incremented integer to compare with the one in context_t. It ensures
  // a context_t instance is always initialized after dxgi_duplicator_controller.
  int identity_ TRAA_GUARDED_BY(mutex_) = 0;
  desktop_rect desktop_rect_ TRAA_GUARDED_BY(mutex_);
  desktop_vector system_dpi_ TRAA_GUARDED_BY(mutex_);
  std::vector<dxgi_adapter_duplicator> duplicators_ TRAA_GUARDED_BY(mutex_);
  d3d_info d3d_info_ TRAA_GUARDED_BY(mutex_);
  display_configuration_monitor display_configuration_monitor_ TRAA_GUARDED_BY(mutex_);
  // A number to indicate how many successful duplications have been performed.
  uint32_t succeeded_duplications_ TRAA_GUARDED_BY(mutex_) = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_WIN_DXGI_DUPLICATOR_CONTROLLER_H_
