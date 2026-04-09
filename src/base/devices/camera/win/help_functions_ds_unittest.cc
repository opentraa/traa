/*
 *  Copyright (c) 2024 The traa authors. All Rights Reserved.
 *
 *  Property-based tests for DirectShow helper functions.
 */

#include "base/devices/camera/win/help_functions_ds.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <objbase.h>
#include <random>
#include <vector>
#include <windows.h>

namespace traa {
namespace base {
namespace {

constexpr int kIterations = 200;

// Test fixture that initializes COM for all tests in this file.
class HelpFunctionsDSTest : public ::testing::Test {
protected:
  void SetUp() override { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
  void TearDown() override { CoUninitialize(); }
};

// ---------------------------------------------------------------------------
// Property 1: copy_media_type deep copy correctness
// Feature: windows-camera-capture, Property 1: copy_media_type deep copy
// **Validates: Requirements 1.6**
// ---------------------------------------------------------------------------

TEST_F(HelpFunctionsDSTest, CopyMediaTypeDeepCopy) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> size_picker(0, 4);
  const ULONG kSizes[] = {0, 1, 10, 100, 1000};
  std::uniform_int_distribution<int> byte_dist(0, 255);

  for (int i = 0; i < kIterations; ++i) {
    // Build a random source AM_MEDIA_TYPE.
    AM_MEDIA_TYPE source = {};
    source.majortype = MEDIATYPE_Video;
    source.subtype = MEDIASUBTYPE_YUY2;
    source.bFixedSizeSamples = TRUE;
    source.bTemporalCompression = FALSE;
    source.lSampleSize = 640 * 480 * 2;
    source.formattype = FORMAT_VideoInfo;
    source.pUnk = nullptr;

    ULONG cb = kSizes[size_picker(rng)];
    source.cbFormat = cb;
    if (cb > 0) {
      source.pbFormat = reinterpret_cast<BYTE *>(CoTaskMemAlloc(cb));
      ASSERT_NE(source.pbFormat, nullptr);
      for (ULONG j = 0; j < cb; ++j) {
        source.pbFormat[j] = static_cast<BYTE>(byte_dist(rng));
      }
    } else {
      source.pbFormat = nullptr;
    }

    // Perform the copy.
    AM_MEDIA_TYPE target = {};
    HRESULT hr = copy_media_type(&target, &source);
    ASSERT_EQ(hr, S_OK) << "copy_media_type failed at iteration " << i;

    // Verify all scalar fields match.
    EXPECT_TRUE(IsEqualGUID(target.majortype, source.majortype));
    EXPECT_TRUE(IsEqualGUID(target.subtype, source.subtype));
    EXPECT_EQ(target.bFixedSizeSamples, source.bFixedSizeSamples);
    EXPECT_EQ(target.bTemporalCompression, source.bTemporalCompression);
    EXPECT_EQ(target.lSampleSize, source.lSampleSize);
    EXPECT_TRUE(IsEqualGUID(target.formattype, source.formattype));
    EXPECT_EQ(target.cbFormat, source.cbFormat);

    // Verify format buffer deep copy.
    if (cb > 0) {
      ASSERT_NE(target.pbFormat, nullptr);
      // Pointers must differ (deep copy).
      EXPECT_NE(target.pbFormat, source.pbFormat)
          << "pbFormat pointers should differ (deep copy) at iteration " << i;
      // Content must match.
      EXPECT_EQ(memcmp(target.pbFormat, source.pbFormat, cb), 0)
          << "pbFormat content mismatch at iteration " << i;
    } else {
      EXPECT_EQ(target.pbFormat, nullptr);
    }

    // Clean up.
    reset_media_type(&target);
    reset_media_type(&source);
  }
}

// ---------------------------------------------------------------------------
// Property 2: com_ref_count reference count invariant
// Feature: windows-camera-capture, Property 2: com_ref_count invariant
// **Validates: Requirements 1.8**
// ---------------------------------------------------------------------------

// Minimal IUnknown-derived class for testing com_ref_count.
class test_unknown : public IUnknown {
public:
  explicit test_unknown(bool *destroyed_flag) : destroyed_flag_(destroyed_flag) {}

  STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override {
    if (riid == IID_IUnknown) {
      *ppv = static_cast<IUnknown *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }

  // AddRef/Release are provided by com_ref_count<test_unknown>.
  STDMETHOD_(ULONG, AddRef)() override { return 0; }
  STDMETHOD_(ULONG, Release)() override { return 0; }

protected:
  ~test_unknown() {
    if (destroyed_flag_)
      *destroyed_flag_ = true;
  }

private:
  bool *destroyed_flag_;
};

TEST_F(HelpFunctionsDSTest, ComRefCountInvariant) {
  std::mt19937 rng(std::random_device{}());
  // Generate sequences of AddRef/Release operations.
  std::uniform_int_distribution<int> seq_len_dist(1, 50);
  std::uniform_int_distribution<int> op_dist(0, 1); // 0 = AddRef, 1 = Release

  for (int i = 0; i < kIterations; ++i) {
    bool destroyed = false;
    auto *obj = new com_ref_count<test_unknown>(&destroyed);
    // Initial ref count is 1.
    ULONG ref_count = 1;

    int seq_len = seq_len_dist(rng);
    // Build a valid sequence: generate random ops but ensure Release count
    // never exceeds current ref count (to keep it non-negative), and we
    // don't release to 0 until the end.
    int add_refs = 0;
    int releases = 0;

    for (int j = 0; j < seq_len; ++j) {
      int op = op_dist(rng);
      if (op == 0) {
        // AddRef
        ULONG new_count = obj->AddRef();
        ref_count++;
        add_refs++;
        EXPECT_EQ(new_count, ref_count)
            << "AddRef count mismatch at iteration " << i << " step " << j;
      } else {
        // Release, but only if ref_count > 1 (don't destroy mid-sequence)
        if (ref_count > 1) {
          ULONG new_count = obj->Release();
          ref_count--;
          releases++;
          EXPECT_EQ(new_count, ref_count)
              << "Release count mismatch at iteration " << i << " step " << j;
          EXPECT_FALSE(destroyed)
              << "Object destroyed prematurely at iteration " << i << " step " << j;
        } else {
          // Can't release further without destroying, do AddRef instead.
          ULONG new_count = obj->AddRef();
          ref_count++;
          add_refs++;
          EXPECT_EQ(new_count, ref_count);
        }
      }
      // Invariant: ref count is always positive during the sequence.
      EXPECT_GT(ref_count, 0u);
    }

    // Now release all remaining refs to destroy the object.
    while (ref_count > 1) {
      ULONG new_count = obj->Release();
      ref_count--;
      EXPECT_EQ(new_count, ref_count);
      EXPECT_FALSE(destroyed);
    }

    // Final release should destroy the object.
    EXPECT_EQ(ref_count, 1u);
    ULONG final_count = obj->Release();
    EXPECT_EQ(final_count, 0u);
    EXPECT_TRUE(destroyed)
        << "Object should be destroyed when ref count reaches 0 at iteration " << i;
  }
}

// ---------------------------------------------------------------------------
// Property 3: get_max_of_frame_array returns minimum
// Feature: windows-camera-capture, Property 3: get_max_of_frame_array returns minimum
// **Validates: Requirements 1.9**
// ---------------------------------------------------------------------------

TEST_F(HelpFunctionsDSTest, GetMaxOfFrameArrayReturnsMinimum) {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<long> size_dist(1, 100);
  std::uniform_int_distribution<LONGLONG> val_dist(1, 1000000);

  for (int i = 0; i < kIterations; ++i) {
    long size = size_dist(rng);
    std::vector<LONGLONG> arr(size);
    for (long j = 0; j < size; ++j) {
      arr[j] = val_dist(rng);
    }

    LONGLONG result = get_max_of_frame_array(arr.data(), size);
    LONGLONG expected = *std::min_element(arr.begin(), arr.end());

    EXPECT_EQ(result, expected)
        << "get_max_of_frame_array should return minimum at iteration " << i;
  }
}

TEST_F(HelpFunctionsDSTest, GetMaxOfFrameArrayEmptyReturnsZero) {
  // Empty array (size = 0) should return 0.
  LONGLONG dummy = 42;
  EXPECT_EQ(get_max_of_frame_array(&dummy, 0), 0);

  // Null pointer should return 0.
  EXPECT_EQ(get_max_of_frame_array(nullptr, 0), 0);
  EXPECT_EQ(get_max_of_frame_array(nullptr, 5), 0);
}

} // namespace
} // namespace base
} // namespace traa
