#ifndef TRAA_BASE_THREAD_THREAD_UTIL_H
#define TRAA_BASE_THREAD_THREAD_UTIL_H

#include "traa/error.h"

#include "base/disallow.h"

#include <cinttypes>

namespace traa {
namespace base {

/**
 * @class thread_util
 *
 * @brief Utility class for working with threads.
 */
class thread_util {
  DISALLOW_IMPLICIT_CONSTRUCTORS(thread_util);

public:
  /**
   * @brief Gets the ID of the current thread.
   *
   * @return The ID of the current thread.
   */
  static std::uintptr_t get_thread_id();

  /**
   * @brief Sets the name of the current thread.
   *
   * @param name The name to set for the thread.
   */
  static void set_thread_name(const char *name);

  /**
   * @brief Allocates a thread local storage key.
   *
   * @param key The key to allocate.
   * @param destructor The destructor to call when the key is freed.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int tls_alloc(std::uintptr_t *key, void (*destructor)(void *) = nullptr);

  /**
   * @brief Sets the value of a thread local storage key.
   *
   * @param key The key to set.
   * @param value The value to set.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int tls_set(std::uintptr_t key, void *value);

  /**
   * @brief Gets the value of a thread local storage key.
   *
   * @param key The key to get.
   *
   * @return The value of the key.
   */
  static void *tls_get(std::uintptr_t key);

  /**
   * @brief Frees a thread local storage key.
   *
   * @param key The key to free.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int tls_free(std::uintptr_t *key);
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_THREAD_UTIL_H