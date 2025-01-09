/*
 *  Copyright (c) 2024 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/darwin/screen_capturer_sck.h"

#include "base/devices/screen/darwin/desktop_frame_iosurface.h"
#include "base/devices/screen/shared_desktop_frame.h"

#include "base/logger.h"

#include "base/sdk/objc/helpers/scoped_cftyperef.h"
#include "base/thread_annotations.h"
#include "base/utils/time_utils.h"

#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <atomic>
#include <mutex>

namespace traa {
namespace base {

class screen_capturer_sck;

} // namespace base
} // namespace traa

// The ScreenCaptureKit API was available in macOS 12.3, but full-screen capture was reported to be
// broken before macOS 13 - see http://crbug.com/40234870.
// Also, the `SCContentFilter` fields `contentRect` and `pointPixelScale` were introduced in
// macOS 14.
API_AVAILABLE(macos(14.0))
@interface sck_helper : NSObject <SCStreamDelegate, SCStreamOutput>

- (instancetype)init_with_capturer:(traa::base::screen_capturer_sck *)capturer;

- (void)on_shareable_content_created:(SCShareableContent *)content;

// Called just before the capturer is destroyed. This avoids a dangling pointer, and prevents any
// new calls into a deleted capturer. If any method-call on the capturer is currently running on a
// different thread, this blocks until it completes.
- (void)release_capturer;

@end

namespace traa {
namespace base {

class API_AVAILABLE(macos(14.0)) screen_capturer_sck final : public desktop_capturer {
public:
  explicit screen_capturer_sck(const desktop_capture_options &options);

  screen_capturer_sck(const screen_capturer_sck &) = delete;
  screen_capturer_sck &operator=(const screen_capturer_sck &) = delete;

  ~screen_capturer_sck() override;

  // DesktopCapturer interface. All these methods run on the caller's thread.
  void start(desktop_capturer::capture_callback *callback) override;
  void set_max_frame_rate(uint32_t max_frame_rate) override;
  void capture_frame() override;
  bool select_source(source_id_t id) override;

  // Called by sck_helper when shareable content is returned by ScreenCaptureKit. `content` will be
  // nil if an error occurred. May run on an arbitrary thread.
  void on_shareable_content_created(SCShareableContent *content);

  // Called by sck_helper to notify of a newly captured frame. May run on an arbitrary thread.
  void on_new_iosurface(IOSurfaceRef io_surface, CFDictionaryRef attachment);

private:
  // Called when starting the capturer or the configuration has changed (either from a
  // select_source() call, or the screen-resolution has changed). This tells SCK to fetch new
  // shareable content, and the completion-handler will either start a new stream, or reconfigure
  // the existing stream. Runs on the caller's thread.
  void start_or_reconfigure_capturer();

  // Helper object to receive Objective-C callbacks from ScreenCaptureKit and call into this C++
  // object. The helper may outlive this C++ instance, if a completion-handler is passed to
  // ScreenCaptureKit APIs and the C++ object is deleted before the handler executes.
  sck_helper *__strong helper_;

  // Callback for returning captured frames, or errors, to the caller. Only used on the caller's
  // thread.
  capture_callback *callback_ = nullptr;

  // Options passed to the constructor. May be accessed on any thread, but the options are
  // unchanged during the capturer's lifetime.
  desktop_capture_options capture_options_;

  // Signals that a permanent error occurred. This may be set on any thread, and is read by
  // CaptureFrame() which runs on the caller's thread.
  std::atomic<bool> permanent_error_ = false;

  // Guards some variables that may be accessed on different threads.
  std::mutex lock_;

  // Provides captured desktop frames.
  SCStream *__strong stream_ TRAA_GUARDED_BY(lock_);

  // Currently selected display, or 0 if the full desktop is selected. This capturer does not
  // support full-desktop capture, and will fall back to the first display.
  CGDirectDisplayID current_display_ TRAA_GUARDED_BY(lock_) = 0;

  // Used by capture_frame() to detect if the screen configuration has changed. Only used on the
  // caller's thread.
  desktop_configuration desktop_config_;

  std::mutex latest_frame_lock_;
  std::unique_ptr<shared_desktop_frame> latest_frame_ TRAA_GUARDED_BY(latest_frame_lock_);

  int32_t latest_frame_dpi_ TRAA_GUARDED_BY(latest_frame_lock_) = desktop_frame::k_standard_dpi;

  // Tracks whether the latest frame contains new data since it was returned to the caller. This is
  // used to set the DesktopFrame's `updated_region` property. The flag is cleared after the frame
  // is sent to OnCaptureResult(), and is set when SCK reports a new frame with non-empty "dirty"
  // rectangles.
  // TODO: crbug.com/327458809 - Replace this flag with ScreenCapturerHelper to more accurately
  // track the dirty rectangles from the SCStreamFrameInfoDirtyRects attachment.
  bool frame_is_dirty_ TRAA_GUARDED_BY(latest_frame_lock_) = true;
};

screen_capturer_sck::screen_capturer_sck(const desktop_capture_options &options)
    : capture_options_(options) {
  helper_ = [[sck_helper alloc] init_with_capturer:this];
}

screen_capturer_sck::~screen_capturer_sck() {
  [stream_ stopCaptureWithCompletionHandler:nil];
  [helper_ release_capturer];
}

void screen_capturer_sck::start(desktop_capturer::capture_callback *callback) {
  callback_ = callback;
  desktop_config_ = capture_options_.configuration_monitor()->get_desktop_configuration();
  start_or_reconfigure_capturer();
}

void screen_capturer_sck::set_max_frame_rate(uint32_t /* max_frame_rate */) {
  // TODO: crbug.com/327458809 - Implement this.
}

void screen_capturer_sck::capture_frame() {
  int64_t capture_start_time_millis = time_millis();

  if (permanent_error_) {
    callback_->on_capture_result(capture_result::error_permanent, nullptr);
    return;
  }

  desktop_configuration new_config =
      capture_options_.configuration_monitor()->get_desktop_configuration();
  if (!desktop_config_.equals(new_config)) {
    desktop_config_ = new_config;
    start_or_reconfigure_capturer();
  }

  std::unique_ptr<desktop_frame> frame;
  {
    std::lock_guard<std::mutex> lock(latest_frame_lock_);
    if (latest_frame_) {
      frame = latest_frame_->share();
      frame->set_dpi(desktop_vector(latest_frame_dpi_, latest_frame_dpi_));
      if (frame_is_dirty_) {
        frame->mutable_updated_region()->add_rect(desktop_rect::make_size(frame->size()));
        frame_is_dirty_ = false;
      }
    }
  }

  if (frame) {
    frame->set_capture_time_ms(time_since(capture_start_time_millis));
    callback_->on_capture_result(capture_result::success, std::move(frame));
  } else {
    callback_->on_capture_result(capture_result::error_temporary, nullptr);
  }
}

bool screen_capturer_sck::select_source(source_id_t id) {
  bool stream_started = false;
  {
    std::lock_guard<std::mutex> lock(lock_);
    current_display_ = static_cast<CGDirectDisplayID>(id);

    if (stream_) {
      stream_started = true;
    }
  }

  // If the capturer was already started, reconfigure it. Otherwise, wait until Start() gets called.
  if (stream_started) {
    start_or_reconfigure_capturer();
  }

  return true;
}

void screen_capturer_sck::on_shareable_content_created(SCShareableContent *content) {
  if (!content) {
    LOG_ERROR("getShareableContent failed.");
    permanent_error_ = true;
    return;
  }

  if (!content.displays.count) {
    LOG_ERROR("getShareableContent returned no displays.");
    permanent_error_ = true;
    return;
  }

  SCDisplay *captured_display;
  {
    std::lock_guard<std::mutex> lock(lock_);
    for (SCDisplay *display in content.displays) {
      if (current_display_ == display.displayID) {
        captured_display = display;
        break;
      }
    }
    if (!captured_display) {
      if (current_display_ == static_cast<CGDirectDisplayID>(k_screen_id_full)) {
        LOG_WARN("Full screen capture is not supported, falling back to first display.");
      } else {
        LOG_WARN("Display {} not found, falling back to first display.", current_display_);
      }
      captured_display = content.displays.firstObject;
    }
  }

  SCContentFilter *filter = [[SCContentFilter alloc] initWithDisplay:captured_display
                                                    excludingWindows:@[]];
  SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
  config.pixelFormat = kCVPixelFormatType_32BGRA;
  config.showsCursor = capture_options_.prefer_cursor_embedded();
  config.width = filter.contentRect.size.width * filter.pointPixelScale;
  config.height = filter.contentRect.size.height * filter.pointPixelScale;
  config.captureResolution = SCCaptureResolutionNominal;

  {
    std::lock_guard<std::mutex> lock(latest_frame_lock_);
    latest_frame_dpi_ = filter.pointPixelScale * desktop_frame::k_standard_dpi;
  }

  std::lock_guard<std::mutex> lock(lock_);

  if (stream_) {
    LOG_INFO("updating stream configuration.");
    [stream_ updateContentFilter:filter completionHandler:nil];
    [stream_ updateConfiguration:config completionHandler:nil];
  } else {
    stream_ = [[SCStream alloc] initWithFilter:filter configuration:config delegate:helper_];

    // TODO: crbug.com/327458809 - Choose an appropriate sampleHandlerQueue for best performance.
    NSError *add_stream_output_error;
    bool add_stream_output_result = [stream_ addStreamOutput:helper_
                                                        type:SCStreamOutputTypeScreen
                                          sampleHandlerQueue:nil
                                                       error:&add_stream_output_error];
    if (!add_stream_output_result) {
      stream_ = nil;
      LOG_ERROR("addStreamOutput failed.");
      permanent_error_ = true;
      return;
    }

    auto handler = ^(NSError *error) {
      if (error) {
        // It should be safe to access `this` here, because the C++ destructor calls
        // stopCaptureWithCompletionHandler on the stream, which cancels this handler.
        permanent_error_ = true;
        LOG_ERROR("startCaptureWithCompletionHandler failed.");
      } else {
        LOG_INFO("capture started.");
      }
    };

    [stream_ startCaptureWithCompletionHandler:handler];
  }
}

void screen_capturer_sck::on_new_iosurface(IOSurfaceRef io_surface, CFDictionaryRef attachment) {
  scoped_cf_type_ref<IOSurfaceRef> scoped_io_surface(io_surface, retain_policy::retain);
  std::unique_ptr<desktop_frame_iosurface> desktop_frame_io_surface =
      desktop_frame_iosurface::wrap(scoped_io_surface);
  if (!desktop_frame_io_surface) {
    LOG_ERROR("failed to lock IOSurface.");
    return;
  }

  std::unique_ptr<shared_desktop_frame> frame =
      shared_desktop_frame::wrap(std::move(desktop_frame_io_surface));

  bool dirty;
  {
    std::lock_guard<std::mutex> lock(latest_frame_lock_);
    // Mark the frame as dirty if it has a different size, and ignore any DirtyRects attachment in
    // this case. This is because SCK does not apply a correct attachment to the frame in the case
    // where the stream was reconfigured.
    dirty = !latest_frame_ || !latest_frame_->size().equals(frame->size());
  }

  if (!dirty) {
    const void *dirty_rects_ptr =
        CFDictionaryGetValue(attachment, (__bridge CFStringRef)SCStreamFrameInfoDirtyRects);
    if (!dirty_rects_ptr) {
      // This is never expected to happen - SCK attaches a non-empty dirty-rects list to every
      // frame, even when nothing has changed.
      return;
    }
    if (CFGetTypeID(dirty_rects_ptr) != CFArrayGetTypeID()) {
      return;
    }

    CFArrayRef dirty_rects_array = static_cast<CFArrayRef>(dirty_rects_ptr);
    int size = static_cast<int>(CFArrayGetCount(dirty_rects_array));
    for (int i = 0; i < size; i++) {
      const void *rect_ptr = CFArrayGetValueAtIndex(dirty_rects_array, i);
      if (CFGetTypeID(rect_ptr) != CFDictionaryGetTypeID()) {
        // This is never expected to happen - the dirty-rects attachment should always be an array
        // of dictionaries.
        return;
      }
      CGRect rect{};
      CGRectMakeWithDictionaryRepresentation(static_cast<CFDictionaryRef>(rect_ptr), &rect);
      if (!CGRectIsEmpty(rect)) {
        dirty = true;
        break;
      }
    }
  }

  if (dirty) {
    std::lock_guard<std::mutex> lock(latest_frame_lock_);
    frame_is_dirty_ = true;
    std::swap(latest_frame_, frame);
  }
}

void screen_capturer_sck::start_or_reconfigure_capturer() {
  // The copy is needed to avoid capturing `this` in the Objective-C block. Accessing `helper_`
  // inside the block is equivalent to `this->helper_` and would crash (UAF) if `this` is
  // deleted before the block is executed.
  sck_helper *local_helper = helper_;
  auto handler = ^(SCShareableContent *content, NSError * /* error */) {
    [local_helper on_shareable_content_created:content];
  };

  [SCShareableContent getShareableContentWithCompletionHandler:handler];
}

std::unique_ptr<desktop_capturer>
create_screen_capture_sck(const desktop_capture_options &options) {
  if (@available(macOS 14.0, *)) {
    return std::make_unique<screen_capturer_sck>(options);
  } else {
    return nullptr;
  }
}

} // namespace base
} // namespace traa

@implementation sck_helper {
  // This lock is to prevent the capturer being destroyed while an instance method is still running
  // on another thread.
  std::mutex _capturer_lock;
  traa::base::screen_capturer_sck *_capturer;
}

- (instancetype)init_with_capturer:(traa::base::screen_capturer_sck *)capturer {
  self = [super init];
  if (self) {
    _capturer = capturer;
  }
  return self;
}

- (void)on_shareable_content_created:(SCShareableContent *)content {
  std::lock_guard<std::mutex> lock(_capturer_lock);
  if (_capturer) {
    _capturer->on_shareable_content_created(content);
  }
}

- (void)stream:(SCStream *)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sample_buffer
                   ofType:(SCStreamOutputType)type {
  CVPixelBufferRef pixel_buffer = CMSampleBufferGetImageBuffer(sample_buffer);
  if (!pixel_buffer) {
    return;
  }

  IOSurfaceRef io_surface = CVPixelBufferGetIOSurface(pixel_buffer);
  if (!io_surface) {
    return;
  }

  CFArrayRef attachments_array =
      CMSampleBufferGetSampleAttachmentsArray(sample_buffer, /*createIfNecessary=*/false);
  if (!attachments_array || CFArrayGetCount(attachments_array) <= 0) {
    LOG_ERROR("Discarding frame with no attachments.");
    return;
  }

  CFDictionaryRef attachment =
      static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments_array, 0));

  std::lock_guard<std::mutex> lock(_capturer_lock);
  if (_capturer) {
    _capturer->on_new_iosurface(io_surface, attachment);
  }
}

- (void)release_capturer {
  std::lock_guard<std::mutex> lock(_capturer_lock);
  _capturer = nullptr;
}

@end
