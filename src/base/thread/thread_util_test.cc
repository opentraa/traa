#include <gtest/gtest.h>

#include "base/platform.h"
#include "base/thread/thread_util.h"

TEST(thread_util_test, thread_name) { traa::base::thread_util::set_thread_name("test thread"); }

TEST(thread_util_test, tls_alloc) {
  std::uintptr_t key = UINTPTR_MAX;
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_alloc(&key));
  EXPECT_GE(key, 0);
  EXPECT_LT(key, UINTPTR_MAX);
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_free(&key));
}

TEST(thread_util_test, tls_set_get) {
  std::uintptr_t key;
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_alloc(&key));

  int value = 9527;
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_set(key, &value));

  int *retrievedValue = static_cast<int *>(traa::base::thread_util::tls_get(key));
  EXPECT_EQ(*retrievedValue, value);

  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_free(&key));

  // set and get after free
  EXPECT_EQ(traa_error::TRAA_ERROR_UNKNOWN, traa::base::thread_util::tls_set(key, &value));
  EXPECT_EQ(nullptr, traa::base::thread_util::tls_get(key));
}

TEST(thread_util_test, tls_free) {
  std::uintptr_t key;

  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_alloc(&key));
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_free(&key));

// free again, expect return error
#if defined(TRAA_OS_WINDOWS)
  // The fucking windows do not return false as they said in the document
  // See:
  // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-tlsfree
  //
  EXPECT_EQ(traa_error::TRAA_ERROR_NONE, traa::base::thread_util::tls_free(&key));
#else
  EXPECT_EQ(traa_error::TRAA_ERROR_UNKNOWN, traa::base::thread_util::tls_free(&key));
#endif
}