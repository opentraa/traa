#ifndef TRAA_BASE_DEVICES_SCREEN_SHARED_MEMORY_H_
#define TRAA_BASE_DEVICES_SCREEN_SHARED_MEMORY_H_

#include "base/platform.h"

#include <stddef.h>

#include <memory>

namespace traa {
namespace base {

// shared_memory is a base class for shared memory. It stores all required
// parameters of the buffer, but doesn't have any logic to allocate or destroy
// the actual buffer. DesktopCapturer consumers that need to use shared memory
// for video frames must extend this class with creation and destruction logic
// specific for the target platform and then call
// DesktopCapturer::SetSharedMemoryFactory().
class shared_memory {
public:
#if defined(TRAA_OS_WINDOWS)
  // Forward declare HANDLE in a windows.h compatible way so that we can avoid
  // including windows.h.
  typedef void *NATIVE_HANDLE;
  static const NATIVE_HANDLE kInvalidHandle;
#else
  typedef int NATIVE_HANDLE;
  static const NATIVE_HANDLE kInvalidHandle;
#endif

  void *data() const { return data_; }
  size_t size() const { return size_; }

  // Platform-specific handle of the buffer.
  NATIVE_HANDLE handle() const { return handle_; }

  // Integer identifier that can be used used by consumers of DesktopCapturer
  // interface to identify shared memory buffers it created.
  int id() const { return id_; }

  virtual ~shared_memory() {}

  shared_memory(const shared_memory &) = delete;
  shared_memory &operator=(const shared_memory &) = delete;

protected:
  shared_memory(void *data, size_t size, NATIVE_HANDLE handle, int id);

  void *const data_;
  const size_t size_;
  const NATIVE_HANDLE handle_;
  const int id_;
};

// Interface used to create shared_memory instances.
class shared_memory_factory {
public:
  shared_memory_factory() {}
  virtual ~shared_memory_factory() {}

  shared_memory_factory(const shared_memory_factory &) = delete;
  shared_memory_factory &operator=(const shared_memory_factory &) = delete;

  virtual std::unique_ptr<shared_memory> CreateSharedMemory(size_t size) = 0;
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_SHARED_MEMORY_H_