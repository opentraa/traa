#include "base/thread/thread_util.h"

#include "base/log/logger.h"
#include "base/strings/string_trans.h"

#include <Windows.h>

namespace traa {
namespace base {

std::uintptr_t thread_util::get_thread_id() { return GetCurrentThreadId(); }

void thread_util::set_thread_name(const char *name) {
  if (!name) {
    return;
  }

  // See
  // https://learn.microsoft.com/en-us/visualstudio/debugger/tips-for-debugging-threads?view=vs-2022&tabs=csharp
  // for more information on setting thread names in Windows.
  const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
  typedef struct tagTHREADNAME_INFO {
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
  } THREADNAME_INFO;
#pragma pack(pop)

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = name;
  info.dwThreadID = GetCurrentThreadId();
  info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
#pragma warning(pop)
}

int thread_util::tls_alloc(std::uintptr_t *key, void (*)(void *)) {
  if (!key) {
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  *key = static_cast<std::uintptr_t>(TlsAlloc());
  if (*key == static_cast<std::uintptr_t>(TLS_OUT_OF_INDEXES)) {
    LOG_ERROR("failed to allocate thread local storage key: {}", GetLastError());
    return traa_error::TRAA_ERROR_RESOURCE_UNAVAILABLE;
  }

  return traa_error::TRAA_ERROR_NONE;
}

int thread_util::tls_set(std::uintptr_t key, void *value) {
  if (!TlsSetValue(static_cast<DWORD>(key), value)) {
    LOG_ERROR("failed to set thread local storage key: {}", GetLastError());
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  return traa_error::TRAA_ERROR_NONE;
}

void *thread_util::tls_get(std::uintptr_t key) { return TlsGetValue(static_cast<DWORD>(key)); }

int thread_util::tls_free(std::uintptr_t *key) {
  if (!TlsFree(static_cast<DWORD>(*key))) {
    LOG_ERROR("failed to free thread local storage key: {}", GetLastError());
    // The fucking windows do not return false when we call TlsFree with valid tls key.
    // They recommand to call TlsFree only during DLL_PROCESS_DETACH.
    // See:
    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-tlsfree
    // If the threads of the process have allocated memory and stored a pointer to the memory in a
    // TLS slot, they should free the memory before calling TlsFree. The TlsFree function does not
    // free memory blocks whose addresses have been stored in the TLS slots associated with the TLS
    // index. It is expected that DLLs call this function (if at all) only during
    // DLL_PROCESS_DETACH.
    //
    // return traa_error::TRAA_ERROR_UNKNOWN;
  }

  *key = static_cast<std::uintptr_t>(TLS_OUT_OF_INDEXES);

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace base
} // namespace traa