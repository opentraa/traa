/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "base/devices/screen/desktop_capturer_differ_wrapper.h"

#include "base/devices/screen/desktop_geometry.h"
#include "base/devices/screen/desktop_region.h"
#include "base/devices/screen/differ_block.h"
#include "base/devices/screen/test/fake_desktop_capturer.h"
#include "base/devices/screen/test/mock_desktop_capturer_callback.h"
#include "base/random.h"
#include "base/utils/time_utils.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace traa {
namespace base {

inline namespace {

// Compares and asserts `frame`.updated_region() equals to `rects`. This
// function does not care about the order of the `rects` and it does not expect
// desktop_region to return an exact area for each rectangle in `rects`.
template <template <typename, typename...> class T = std::initializer_list, typename... Rect>
void assert_updated_region_is(const desktop_frame &frame, const T<desktop_rect, Rect...> &rects) {
  desktop_region region;
  for (const auto &rect : rects) {
    region.add_rect(rect);
  }
  ASSERT_TRUE(frame.updated_region().equals(region));
}

// Compares and asserts `frame`.updated_region() covers all rectangles in
// `rects`, but does not cover areas other than a k_differ_block_size expansion. This
// function does not care about the order of the `rects`, and it does not expect
// desktop_region to return an exact area of each rectangle in `rects`.
template <template <typename, typename...> class T = std::initializer_list, typename... Rect>
void assert_updated_region_covers(const desktop_frame &frame,
                                  const T<desktop_rect, Rect...> &rects) {
  desktop_region region;
  for (const auto &rect : rects) {
    region.add_rect(rect);
  }

  // Intersect of `rects` and `frame`.updated_region() should be `rects`. i.e.
  // `frame`.updated_region() should be a superset of `rects`.
  desktop_region intersect(region);
  intersect.intersect_with(frame.updated_region());
  ASSERT_TRUE(region.equals(intersect));

  // Difference between `rects` and `frame`.updated_region() should not cover
  // areas which have larger than twice of k_differ_block_size width and height.
  //
  // Explanation of the 'twice' of k_differ_block_size (indeed k_differ_block_size * 2 - 2) is
  // following,
  // (Each block in the following grid is a 8 x 8 pixels area. X means the real
  // updated area, m means the updated area marked by
  // desktop_capturer_differ_wrapper.)
  // +---+---+---+---+---+---+---+---+
  // | X | m | m | m | m | m | m | m |
  // +---+---+---+---+---+---+---+---+
  // | m | m | m | m | m | m | m | m |
  // +---+---+---+---+---+---+---+---+
  // | m | m | m | m | m | m | m | m |
  // +---+---+---+---+---+---+---+---+
  // | m | m | m | m | m | m | m | X |
  // +---+---+---+---+---+---+---+---+
  // The top left [0, 0] - [8, 8] and right bottom [56, 24] - [64, 32] blocks of
  // this area are updated. But since desktop_capturer_differ_wrapper compares
  // 32 x 32 blocks by default, this entire area is marked as updated. So the
  // [8, 8] - [56, 32] is expected to be covered in the difference.
  //
  // But if [0, 0] - [8, 8] and [64, 24] - [72, 32] blocks are updated,
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | X | m | m | m |   |   |   |   | m | m | m | m |
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | m | m | m | m |   |   |   |   | m | m | m | m |
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | m | m | m | m |   |   |   |   | m | m | m | m |
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // | m | m | m | m |   |   |   |   | X | m | m | m |
  // +---+---+---+---+---+---+---+---+---+---+---+---+
  // the [8, 8] - [64, 32] is not expected to be covered in the difference. As
  // desktop_capturer_differ_wrapper should only mark [0, 0] - [32, 32] and
  // [64, 0] - [96, 32] as updated.
  desktop_region differ(frame.updated_region());
  differ.subtract(region);
  for (desktop_region::iterator it(differ); !it.is_at_end(); it.advance()) {
    ASSERT_TRUE(it.rect().width() <= k_differ_block_size * 2 - 2 ||
                it.rect().height() <= k_differ_block_size * 2 - 2);
  }
}

// Executes a desktop_capturer_differ_wrapper::capture() and compares its output
// desktop_frame::updated_region() with `updated_region` if `check_result` is
// true. If `exactly_match` is true, assert_updated_region_is() will be used,
// otherwise assert_updated_region_covers() will be used.
template <template <typename, typename...> class T = std::initializer_list, typename... Rect>
void execute_differ_wrapper_case(black_white_desktop_frame_painter *frame_painter,
                                 desktop_capturer_differ_wrapper *capturer,
                                 mock_desktop_capturer_callback *callback,
                                 const T<desktop_rect, Rect...> &updated_region, bool check_result,
                                 bool exactly_match) {
  EXPECT_CALL(*callback,
              on_capture_result_ptr(desktop_capturer::capture_result::success, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke(
          [&updated_region, check_result, exactly_match](desktop_capturer::capture_result result,
                                                         std::unique_ptr<desktop_frame> *frame) {
            ASSERT_EQ(result, desktop_capturer::capture_result::success);
            if (check_result) {
              if (exactly_match) {
                assert_updated_region_is(**frame, updated_region);
              } else {
                assert_updated_region_covers(**frame, updated_region);
              }
            }
          }));
  for (const auto &rect : updated_region) {
    frame_painter->updated_region()->add_rect(rect);
  }
  capturer->capture_frame();
}

// Executes a desktop_capturer_differ_wrapper::capture(), if updated_region() is
// not set, this function will reset desktop_capturer_differ_wrapper internal
// desktop_frame into black.
void execute_capturer(desktop_capturer_differ_wrapper *capturer,
                      mock_desktop_capturer_callback *callback) {
  EXPECT_CALL(*callback,
              on_capture_result_ptr(desktop_capturer::capture_result::success, ::testing::_))
      .Times(1);
  capturer->capture_frame();
}

void execute_differ_wrapper_test(bool with_hints, bool enlarge_updated_region,
                                 bool random_updated_region, bool check_result) {
  const bool updated_region_should_exactly_match =
      with_hints && !enlarge_updated_region && !random_updated_region;
  black_white_desktop_frame_painter frame_painter;
  painter_desktop_frame_generator frame_generator;
  frame_generator.set_desktop_frame_painter(&frame_painter);
  std::unique_ptr<fake_desktop_capturer> fake(new fake_desktop_capturer());
  fake->set_frame_generator(&frame_generator);
  desktop_capturer_differ_wrapper capturer(std::move(fake));
  mock_desktop_capturer_callback callback;
  frame_generator.set_provide_updated_region_hints(with_hints);
  frame_generator.set_enlarge_updated_region(enlarge_updated_region);
  frame_generator.set_add_random_updated_region(random_updated_region);

  capturer.start(&callback);

  EXPECT_CALL(callback,
              on_capture_result_ptr(desktop_capturer::capture_result::success, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke(
          [](desktop_capturer::capture_result result, std::unique_ptr<desktop_frame> *frame) {
            ASSERT_EQ(result, desktop_capturer::capture_result::success);
            assert_updated_region_is(**frame, {desktop_rect::make_size((*frame)->size())});
          }));
  capturer.capture_frame();

  execute_differ_wrapper_case(
      &frame_painter, &capturer, &callback,
      {desktop_rect::make_ltrb(100, 100, 200, 200), desktop_rect::make_ltrb(300, 300, 400, 400)},
      check_result, updated_region_should_exactly_match);
  execute_capturer(&capturer, &callback);

  execute_differ_wrapper_case(
      &frame_painter, &capturer, &callback,
      {desktop_rect::make_ltrb(0, 0, 40, 40),
       desktop_rect::make_ltrb(0, frame_generator.size()->height() - 40, 40,
                               frame_generator.size()->height()),
       desktop_rect::make_ltrb(frame_generator.size()->width() - 40, 0,
                               frame_generator.size()->width(), 40),
       desktop_rect::make_ltrb(frame_generator.size()->width() - 40,
                               frame_generator.size()->height() - 40,
                               frame_generator.size()->width(), frame_generator.size()->height())},
      check_result, updated_region_should_exactly_match);

  base::random rd(time_millis());
  // Fuzzing tests.
  for (int i = 0; i < 1000; i++) {
    if (enlarge_updated_region) {
      frame_generator.set_enlarge_range(rd.rand(1, 50));
    }
    frame_generator.size()->set(rd.rand(500, 2000), rd.rand(500, 2000));
    execute_capturer(&capturer, &callback);
    std::vector<desktop_rect> updated_region;
    for (int j = rd.rand(50); j >= 0; j--) {
      // At least a 1 x 1 updated region.
      const int left = rd.rand(0, frame_generator.size()->width() - 2);
      const int top = rd.rand(0, frame_generator.size()->height() - 2);
      const int right = rd.rand(left + 1, frame_generator.size()->width());
      const int bottom = rd.rand(top + 1, frame_generator.size()->height());
      updated_region.push_back(desktop_rect::make_ltrb(left, top, right, bottom));
    }
    execute_differ_wrapper_case(&frame_painter, &capturer, &callback, updated_region, check_result,
                                updated_region_should_exactly_match);
  }
}

} // namespace

TEST(desktop_capturer_differ_wrapper_test, capture_without_hints) {
  execute_differ_wrapper_test(false, false, false, true);
}

TEST(desktop_capturer_differ_wrapper_test, capture_with_hints) {
  execute_differ_wrapper_test(true, false, false, true);
}

TEST(desktop_capturer_differ_wrapper_test, capture_with_enlarged_hints) {
  execute_differ_wrapper_test(true, true, false, true);
}

TEST(desktop_capturer_differ_wrapper_test, capture_with_random_hints) {
  execute_differ_wrapper_test(true, false, true, true);
}

TEST(desktop_capturer_differ_wrapper_test, capture_with_enlarged_and_random_hints) {
  execute_differ_wrapper_test(true, true, true, true);
}

// When hints are provided, desktop_capturer_differ_wrapper has a slightly better
// performance in current configuration, but not so significant. Following is
// one run result.
// [ RUN      ] DISABLED_CaptureWithoutHintsPerf
// [       OK ] DISABLED_CaptureWithoutHintsPerf (7118 ms)
// [ RUN      ] DISABLED_CaptureWithHintsPerf
// [       OK ] DISABLED_CaptureWithHintsPerf (5580 ms)
// [ RUN      ] DISABLED_CaptureWithEnlargedHintsPerf
// [       OK ] DISABLED_CaptureWithEnlargedHintsPerf (5974 ms)
// [ RUN      ] DISABLED_CaptureWithRandomHintsPerf
// [       OK ] DISABLED_CaptureWithRandomHintsPerf (6184 ms)
// [ RUN      ] DISABLED_CaptureWithEnlargedAndRandomHintsPerf
// [       OK ] DISABLED_CaptureWithEnlargedAndRandomHintsPerf (6347 ms)
TEST(desktop_capturer_differ_wrapper_test, DISABLED_capture_without_hints_perf) {
  int64_t started = time_millis();
  execute_differ_wrapper_test(false, false, false, false);
  ASSERT_LE(time_millis() - started, 15000);
}

TEST(desktop_capturer_differ_wrapper_test, DISABLED_capture_with_hints_perf) {
  int64_t started = time_millis();
  execute_differ_wrapper_test(true, false, false, false);
  ASSERT_LE(time_millis() - started, 15000);
}

TEST(desktop_capturer_differ_wrapper_test, DISABLED_capture_with_enlarged_hints_perf) {
  int64_t started = time_millis();
  execute_differ_wrapper_test(true, true, false, false);
  ASSERT_LE(time_millis() - started, 15000);
}

TEST(desktop_capturer_differ_wrapper_test, DISABLED_capture_with_random_hints_perf) {
  int64_t started = time_millis();
  execute_differ_wrapper_test(true, false, true, false);
  ASSERT_LE(time_millis() - started, 15000);
}

TEST(desktop_capturer_differ_wrapper_test, DISABLED_capture_with_enlarged_and_random_hints_perf) {
  int64_t started = time_millis();
  execute_differ_wrapper_test(true, true, true, false);
  ASSERT_LE(time_millis() - started, 15000);
}

} // namespace base
} // namespace traa