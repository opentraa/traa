#include "base/thread/rw_lock.h"

#include "base/logger.h"

#include <synchapi.h>

// Slim Reader/Writer (SRW) Locks are a lightweight synchronization primitive that can be used to
// protect shared data structures from concurrent access. SRW locks provide a more efficient
// synchronization mechanism than critical sections and mutexes. SRW locks are the recommended
// synchronization primitive for reader/writer scenarios in user mode. SRW locks are available on
// Windows Vista and later. For Windows XP and Windows Server 2003, use the
// InitializeCriticalSectionAndSpinCount and DeleteCriticalSection functions.
// More information:
// https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks

namespace traa {
namespace base {

rw_lock::rw_lock() : rw_lock_(new SRWLOCK) { InitializeSRWLock(static_cast<PSRWLOCK>(rw_lock_)); }

rw_lock::~rw_lock() { delete static_cast<PSRWLOCK>(rw_lock_); }

bool rw_lock::read_lock() {
  AcquireSRWLockShared(static_cast<PSRWLOCK>(rw_lock_));
  return true;
}

bool rw_lock::try_read_lock() { return TryAcquireSRWLockShared(static_cast<PSRWLOCK>(rw_lock_)); }

void rw_lock::read_unlock() { ReleaseSRWLockShared(static_cast<PSRWLOCK>(rw_lock_)); }

bool rw_lock::write_lock() {
  AcquireSRWLockExclusive(static_cast<PSRWLOCK>(rw_lock_));
  return true;
}

bool rw_lock::try_write_lock() {
  return TryAcquireSRWLockExclusive(static_cast<PSRWLOCK>(rw_lock_));
}

void rw_lock::write_unlock() { ReleaseSRWLockExclusive(static_cast<PSRWLOCK>(rw_lock_)); }

} // namespace base
} // namespace traa