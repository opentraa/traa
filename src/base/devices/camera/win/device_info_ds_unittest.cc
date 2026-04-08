/*
 *  Copyright (c) 2024 The traa authors. All Rights Reserved.
 *
 *  Property-based tests for device_info_ds.
 */

#include "base/devices/camera/win/device_info_ds.h"

#include <gtest/gtest.h>

#include <cstring>
#include <random>
#include <string>

namespace traa {
namespace base {
namespace {

constexpr int kIterations = 200;

// ---------------------------------------------------------------------------
// Property 7: get_product_id device path parsing
// Feature: windows-camera-capture, Property 7: get_product_id device path parsing
// **Validates: Requirements 4.13**
//
// For any string matching the Windows DevicePath format
// (\\?\<bus>#<vid>&<pid>&<rest>#...), get_product_id should extract the
// substring from the first # after \\?\ to the second &.
// For paths without the \\?\ prefix, it should return an empty string.
// ---------------------------------------------------------------------------

// Helper: generate a random alphanumeric string of given length.
static std::string random_alnum(std::mt19937 &rng, int min_len, int max_len) {
  static const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789_-";
  std::uniform_int_distribution<int> len_dist(min_len, max_len);
  std::uniform_int_distribution<int> char_dist(0, static_cast<int>(sizeof(chars) - 2));
  int len = len_dist(rng);
  std::string s;
  s.reserve(len);
  for (int i = 0; i < len; ++i) {
    s += chars[char_dist(rng)];
  }
  return s;
}

TEST(DeviceInfoDSTest, GetProductIdExtractsCorrectly) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    // Generate random components for a DevicePath:
    // \\?\<bus>#<vid>&<pid>&<rest>#<serial>#{<guid>}\global
    std::string bus = random_alnum(rng, 2, 10);
    std::string vid = random_alnum(rng, 3, 20);
    std::string pid = random_alnum(rng, 3, 20);
    std::string rest = random_alnum(rng, 1, 10);
    std::string serial = random_alnum(rng, 4, 20);
    std::string guid = random_alnum(rng, 10, 30);

    // Build the device path.
    std::string device_path =
        "\\\\?\\" + bus + "#" + vid + "&" + pid + "&" + rest + "#" + serial + "#{" + guid + "}";

    // The expected product ID is from after \\?\ (i.e., from <bus>#<vid>&<pid>)
    // which is: from start_pos (bus#vid&pid) up to the second &.
    // start_pos points to <bus>, first & is between vid and pid,
    // second & is between pid and rest.
    // So expected = bus + "#" + vid + "&" + pid
    std::string expected = bus + "#" + vid + "&" + pid;

    char result[256] = {};
    device_info_ds::get_product_id(device_path.c_str(), result, sizeof(result));

    EXPECT_STREQ(result, expected.c_str())
        << "Failed at iteration " << i << " with path: " << device_path;
  }
}

TEST(DeviceInfoDSTest, GetProductIdNoPrefixReturnsEmpty) {
  std::mt19937 rng(std::random_device{}());

  for (int i = 0; i < kIterations; ++i) {
    // Generate paths that do NOT start with \\?\  .
    // Use various prefixes that are not the expected one.
    std::string path = random_alnum(rng, 5, 50);
    // Make sure it doesn't accidentally contain \\?\  .
    // Replace any backslashes.
    for (char &c : path) {
      if (c == '\\')
        c = '/';
    }

    char result[256] = {};
    device_info_ds::get_product_id(path.c_str(), result, sizeof(result));

    EXPECT_STREQ(result, "") << "Expected empty string for path without \\\\?\\ prefix at "
                                "iteration "
                             << i << " with path: " << path;
  }
}

TEST(DeviceInfoDSTest, GetProductIdSingleAmpersandReturnsEmpty) {
  // Path with \\?\ prefix but only one & (no second & to delimit product ID).
  const char *path = "\\\\?\\usb#vid_0408&only_one_segment#serial";
  char result[256] = {};
  // After \\?\, start_pos = "usb#vid_0408&only_one_segment#serial"
  // First & found at "vid_0408&only_one_segment#serial"
  // Second & search from "only_one_segment#serial" — no & found, pos is null.
  // But actually strchr will find '#' is not '&', so pos will be null.
  // The function should return empty.

  // Actually let's trace: start_pos = "usb#vid_0408&only_one_segment#serial"
  // First strchr for '&' finds the one between vid_0408 and only_one_segment.
  // Second strchr(pos+1, '&') searches "only_one_segment#serial" for '&' — not found, returns null.
  // So pos is null, and the else branch sets empty string.
  device_info_ds::get_product_id(path, result, sizeof(result));
  EXPECT_STREQ(result, "");
}

} // namespace
} // namespace base
} // namespace traa
