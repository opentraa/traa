#include "base/thread/rw_lock.h"

#include "base/log/logger.h"

#include <pthread.h>

namespace traa {
namespace base {

rw_lock::rw_lock() : rw_lock_(new pthread_rwlock_t) {
  if (pthread_rwlock_init(static_cast<pthread_rwlock_t *>(rw_lock_), nullptr) != 0) {
    LOG_ERROR("Failed to initialize rw_lock.");
    delete static_cast<pthread_rwlock_t *>(rw_lock_);
    rw_lock_ = nullptr;
  }
}

rw_lock::~rw_lock() {
  pthread_rwlock_destroy(static_cast<pthread_rwlock_t *>(rw_lock_));
  delete static_cast<pthread_rwlock_t *>(rw_lock_);
}

bool rw_lock::read_lock() {
  return pthread_rwlock_rdlock(static_cast<pthread_rwlock_t *>(rw_lock_)) == 0;
}

bool rw_lock::try_read_lock() {
  return pthread_rwlock_tryrdlock(static_cast<pthread_rwlock_t *>(rw_lock_)) == 0;
}

void rw_lock::read_unlock() { pthread_rwlock_unlock(static_cast<pthread_rwlock_t *>(rw_lock_)); }

bool rw_lock::write_lock() {
  return pthread_rwlock_wrlock(static_cast<pthread_rwlock_t *>(rw_lock_)) == 0;
}

bool rw_lock::try_write_lock() {
  return pthread_rwlock_trywrlock(static_cast<pthread_rwlock_t *>(rw_lock_)) == 0;
}

void rw_lock::write_unlock() { pthread_rwlock_unlock(static_cast<pthread_rwlock_t *>(rw_lock_)); }

} // namespace base
} // namespace traa