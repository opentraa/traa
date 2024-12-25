/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TRAA_BASE_DEVICES_SCREEN_TEST_DESKTOP_FRAME_GENERATOR_H_
#define TRAA_BASE_DEVICES_SCREEN_TEST_DESKTOP_FRAME_GENERATOR_H_

#include "base/devices/screen/desktop_frame.h"
#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/shared_memory.h"

#include <memory>

namespace traa {
namespace base {

// An interface to generate a desktop_frame.
class desktop_frame_generator {
public:
  desktop_frame_generator();
  virtual ~desktop_frame_generator();

  virtual std::unique_ptr<desktop_frame> get_next_frame(shared_memory_factory *factory) = 0;
};

// An interface to paint a desktop_frame. This interface is used by
// painter_desktop_frame_generator.
class desktop_frame_painter {
public:
  desktop_frame_painter();
  virtual ~desktop_frame_painter();

  virtual bool paint(desktop_frame *frame, desktop_region *updated_region) = 0;
};

// An implementation of desktop_frame_generator to take care about the
// desktop_frame size, filling updated_region(), etc, but leaves the real
// painting work to a desktop_frame_painter implementation.
class painter_desktop_frame_generator final : public desktop_frame_generator {
public:
  painter_desktop_frame_generator();
  ~painter_desktop_frame_generator() override;

  std::unique_ptr<desktop_frame> get_next_frame(shared_memory_factory *factory) override;

  // Sets the size of the frame which will be returned in next get_next_frame()
  // call.
  desktop_size *size();

  // Decides whether base_desktop_capturer returns a frame in next Capture()
  // callback. If return_frame_ is true, base_desktop_capturer will create a
  // frame according to both size_ and shared_memory_factory input, and uses
  // paint() function to paint it.
  void set_return_frame(bool return_frame);

  // Decides whether mock_desktop_capturer returns a frame with updated regions.
  // mock_desktop_capturer will keep desktop_frame::updated_region() empty if this
  // field is false.
  void set_provide_updated_region_hints(bool provide_updated_region_hints);

  // Decides whether mock_desktop_capturer randomly enlarges updated regions in the
  // desktop_frame. Set this field to true to simulate an inaccurate updated
  // regions' return from OS APIs.
  void set_enlarge_updated_region(bool enlarge_updated_region);

  // The range to enlarge a updated region if `enlarge_updated_region_` is true.
  // If this field is less than zero, it will be treated as zero, and
  // `enlarge_updated_region_` will be ignored.
  void set_enlarge_range(int enlarge_range);

  // Decides whether base_desktop_capturer randomly add some updated regions
  // in the desktop_frame. Set this field to true to simulate an inaccurate
  // updated regions' return from OS APIs.
  void set_add_random_updated_region(bool add_random_updated_region);

  // Sets the painter object to do the real painting work, if no `painter_` has
  // been set to this instance, the desktop_frame returned by get_next_frame()
  // function will keep in an undefined but valid state.
  // painter_desktop_frame_generator does not take ownership of the `painter`.
  void set_desktop_frame_painter(desktop_frame_painter *painter);

private:
  desktop_size size_;
  bool return_frame_;
  bool provide_updated_region_hints_;
  bool enlarge_updated_region_;
  int enlarge_range_;
  bool add_random_updated_region_;
  desktop_frame_painter *painter_;
};

// An implementation of desktop_frame_painter to paint black on
// mutable_updated_region(), and white elsewhere.
class black_white_desktop_frame_painter final : public desktop_frame_painter {
public:
  black_white_desktop_frame_painter();
  ~black_white_desktop_frame_painter() override;

  // The black regions of the frame which will be returned in next paint()
  // call. black_white_desktop_frame_painter will draw a white frame, with black
  // in the updated_region_. Each paint() call will consume updated_region_.
  desktop_region *updated_region();

  // desktop_frame_painter interface.
  bool paint(desktop_frame *frame, desktop_region *updated_region) override;

private:
  desktop_region updated_region_;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_TEST_DESKTOP_FRAME_GENERATOR_H_
