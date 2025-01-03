#include "base/thread/thread_util.h"

#include "base/logger.h"

#include <pthread.h>

#include <sys/syscall.h>

namespace traa {
namespace base {

std::uintptr_t thread_util::get_thread_id() {
#if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#define SYS_gettid __NR_gettid
#endif
  return static_cast<std::uintptr_t>(::syscall(SYS_gettid));
}

void thread_util::set_thread_name(const char *name) {
  if (!name) {
    return;
  }

  // Set the thread name for the current thread.
  int ret = pthread_setname_np(pthread_self(), name);
  if (ret != 0) {
    LOG_ERROR("failed to set thread name: {} for current thread", name);
  }
}

int thread_util::tls_alloc(std::uintptr_t *key, void (*destructor)(void *)) {
  if (!key) {
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  pthread_key_t key_t;
  int ret = pthread_key_create(&key_t, destructor);
  if (ret != 0) {
    LOG_ERROR("failed to allocate thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  *key = static_cast<std::uintptr_t>(key_t);

  return traa_error::TRAA_ERROR_NONE;
}

int thread_util::tls_set(std::uintptr_t key, void *value) {
  int ret = pthread_setspecific(static_cast<pthread_key_t>(key), value);
  if (ret != 0) {
    LOG_ERROR("failed to set thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

void *thread_util::tls_get(std::uintptr_t key) {
  return pthread_getspecific(static_cast<pthread_key_t>(key));
}

int thread_util::tls_free(std::uintptr_t *key) {
  int ret = pthread_key_delete(static_cast<pthread_key_t>(*key));
  if (ret != 0) {
    LOG_ERROR("failed to free thread local storage key: {}", ret);
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa