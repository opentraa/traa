#include "base/thread/thread_util.h"

#include "base/log/logger.h"

#include <AvailabilityMacros.h>

#include <pthread.h>

namespace traa {
namespace base {

std::uintptr_t thread_util::get_thread_id() {
  uint64_t tid;
// There is no pthread_threadid_np prior to Mac OS X 10.6, and it is not supported on any PPC,
// including 10.6.8 Rosetta. __POWERPC__ is Apple-specific define encompassing ppc and ppc64.
#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
  {
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060) || defined(__POWERPC__)
    tid = pthread_mach_thread_np(pthread_self());
#elif MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    if (&pthread_threadid_np) {
      pthread_threadid_np(nullptr, &tid);
    } else {
      tid = pthread_mach_thread_np(pthread_self());
    }
#else
    pthread_threadid_np(nullptr, &tid);
#endif
  }
#else
  pthread_threadid_np(nullptr, &tid);
#endif

  return static_cast<std::uintptr_t>(tid);
}

void thread_util::set_thread_name(const char *name) {
  if (!name) {
    return;
  }

  // Set the thread name for the current thread.
  int ret = pthread_setname_np(name);
  if (ret != 0) {
    LOG_ERROR("failed to set thread name: {} for current thread", name);
  }
}

int thread_util::tls_alloc(std::uintptr_t *key, void (*destructor)(void *)) {
  if (!key) {
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  int ret = pthread_key_create(key, destructor);
  if (ret != 0) {
    LOG_ERROR("failed to allocate thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

int thread_util::tls_set(std::uintptr_t key, void *value) {
  int ret = pthread_setspecific(key, value);
  if (ret != 0) {
    LOG_ERROR("failed to set thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

void *thread_util::tls_get(std::uintptr_t key) { return pthread_getspecific(key); }

int thread_util::tls_free(std::uintptr_t key) {
  int ret = pthread_key_delete(key);
  if (ret != 0) {
    LOG_ERROR("failed to free thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa