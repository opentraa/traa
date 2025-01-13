/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_CAPTURER_H_
#define TRAA_BASE_DEVICES_SCREEN_CAPTURER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "base/devices/screen/delegated_source_list_controller.h"
#include "base/devices/screen/desktop_capture_types.h"
#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/shared_memory.h"

namespace traa {
namespace base {

void log_desktop_capturer_fullscreen_detector_usage();

class desktop_capture_options;
class desktop_frame;

// Abstract interface for screen and window capturers.
class desktop_capturer {
public:
  enum class capture_result {
    // The frame was captured successfully.
    success,

    // There was a temporary error. The caller should continue calling
    // capture_frame(), in the expectation that it will eventually recover.
    error_temporary,

    // Capture has failed and will keep failing if the caller tries calling
    // capture_frame() again.
    error_permanent,

    max_value = error_permanent
  };

  // Interface that must be implemented by the desktop_capturer consumers.
  class capture_callback {
  public:
    // Called before a frame capture is started.
    virtual void on_capture_start() {}

    // Called after a frame has been captured. `frame` is not nullptr if and
    // only if `result` is SUCCESS.
    virtual void on_capture_result(capture_result result, std::unique_ptr<desktop_frame> frame) = 0;

  protected:
    virtual ~capture_callback() {}
  };

  using source_id_t = intptr_t;

  static_assert(std::is_same<source_id_t, screen_id_t>::value,
                "source_id_t should be a same type as screen_id_t.");

  typedef struct source_t_ {
    // The unique id to represent a source of current desktop_capturer.
    source_id_t id;

    // Title of the window or screen in UTF-8 encoding, maybe empty. This field
    // should not be used to identify a source.
    std::string title;

    // The display's unique ID. If no ID is defined, it will hold the value
    // k_display_id_invalid.
    int64_t display_id = k_display_id_invalid;
  } source_t;

  using source_list_t = std::vector<source_t>;

  virtual ~desktop_capturer();

  // Returns the capturer id.
  virtual uint32_t current_capturer_id() const { return desktop_capture_id::k_capture_unknown; }

  // Called at the beginning of a capturing session. `callback` must remain
  // valid until capturer is destroyed.
  virtual void start(capture_callback *callback) = 0;

  // Sets max frame rate for the capturer. This is best effort and may not be
  // supported by all capturers. This will only affect the frequency at which
  // new frames are available, not the frequency at which you are allowed to
  // capture the frames.
  virtual void set_max_frame_rate(uint32_t max_frame_rate) {}

  // Returns a valid pointer if the capturer requires the user to make a
  // selection from a source list provided by the capturer.
  // Returns nullptr if the capturer does not provide a UI for the user to make
  // a selection.
  //
  // Callers should not take ownership of the returned pointer, but it is
  // guaranteed to be valid as long as the desktop_capturer is valid.
  // Note that consumers should still use GetSourceList and select_source, but
  // their behavior may be modified if this returns a value. See those methods
  // for a more in-depth discussion of those potential modifications.
  virtual delegated_source_list_controller *get_delegated_source_list_controller();

  // Sets shared_memory_factory that will be used to create buffers for the
  // captured frames. The factory can be invoked on a thread other than the one
  // where capture_frame() is called. It will be destroyed on the same thread.
  // Shared memory is currently supported only by some desktop_capturer
  // implementations.
  virtual void
  set_shared_memory_factory(std::unique_ptr<shared_memory_factory> shared_memory_factory);

  // Captures next frame, and involve callback provided by Start() function.
  // Pending capture requests are canceled when desktop_capturer is deleted.
  virtual void capture_frame() = 0;

  // Sets the window to be excluded from the captured image in the future
  // Capture calls. Used to exclude the screenshare notification window for
  // screen capturing.
  virtual void set_excluded_window(win_id_t window);

  // TODO(zijiehe): Following functions should be pure virtual. The default
  // implementations are for backward compatibility only. Remove default
  // implementations once all desktop_capturer implementations in Chromium have
  // implemented these functions.

  // Gets a list of sources current capturer supports. Returns false in case of
  // a failure.
  // For desktop_capturer implementations to capture screens, this function
  // should return monitors.
  // For desktop_capturer implementations to capture windows, this function
  // should only return root windows owned by applications.
  //
  // Note that capturers who use a delegated source list will return a
  // source_list_t with exactly one value, but it may not be viable for capture
  // (e.g. capture_frame will return ERROR_TEMPORARY) until a selection has been
  // made.
  virtual bool get_source_list(source_list_t *sources);

  // Selects a source to be captured. Returns false in case of a failure (e.g.
  // if there is no source with the specified type and id.)
  //
  // Note that some capturers with delegated source lists may also support
  // selecting a SourceID that is not in the returned source list as a form of
  // restore token.
  virtual bool select_source(source_id_t id);

  // Brings the selected source to the front and sets the input focus on it.
  // Returns false in case of a failure or no source has been selected or the
  // implementation does not support this functionality.
  virtual bool focus_on_selected_source();

  // Returns true if the `pos` on the selected source is covered by other
  // elements on the display, and is not visible to the users.
  // `pos` is in full desktop coordinates, i.e. the top-left monitor always
  // starts from (0, 0).
  // The return value if `pos` is out of the scope of the source is undefined.
  virtual bool is_occluded(const desktop_vector &pos);

  // Creates a desktop_capturer instance which targets to capture windows.
  static std::unique_ptr<desktop_capturer>
  create_window_capturer(const desktop_capture_options &options);

  // Creates a desktop_capturer instance which targets to capture screens.
  static std::unique_ptr<desktop_capturer>
  create_screen_capturer(const desktop_capture_options &options);

  // Creates a desktop_capturer instance which targets to capture windows and
  // screens.
  static std::unique_ptr<desktop_capturer>
  create_generic_capturer(const desktop_capture_options &options);

#if defined(TRAA_ENABLE_X11) || defined(TRAA_ENABLE_WAYLAND)
  virtual void update_resolution(uint32_t width, uint32_t height) {}
#endif // defined(TRAA_ENABLE_X11) || defined(TRAA_ENABLE_WAYLAND)

#if defined(TRAA_ENABLE_WAYLAND)
  // Populates implementation specific metadata into the passed in pointer.
  // Classes can choose to override it or use the default no-op implementation.
  virtual desktop_capture_metadata get_metadata() { return {}; }
#endif // defined(TRAA_ENABLE_WAYLAND)

protected:
  // CroppingWindowCapturer needs to create raw capturers without wrappers, so
  // the following two functions are protected.

  // Creates a platform specific desktop_capturer instance which targets to
  // capture windows.
  static std::unique_ptr<desktop_capturer>
  create_raw_window_capturer(const desktop_capture_options &options);

  // Creates a platform specific desktop_capturer instance which targets to
  // capture screens.
  static std::unique_ptr<desktop_capturer>
  create_raw_screen_capturer(const desktop_capture_options &options);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_CAPTURER_H_
