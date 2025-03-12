#ifndef TRAA_BASE_DEVICES_SCREEN_ENUMERATOR_H_
#define TRAA_BASE_DEVICES_SCREEN_ENUMERATOR_H_

#include <traa/base.h>

#include "base/disallow.h"

#include <string>
#include <vector>

/**
 * @brief The `screen_source_info_enumerator` class provides functionality for enumerating and
 * managing screen source information.
 *
 * This class allows you to retrieve screen source information, such as icon size, thumbnail size,
 * and the number of screen source information available. It also provides a method to free the
 * screen source information when it is no longer needed.
 */
namespace traa {
namespace base {

class screen_source_info_enumerator {
  DISALLOW_IMPLICIT_CONSTRUCTORS(screen_source_info_enumerator);

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
public:
  /**
   * @brief Gets the screen source information.
   *
   * @param icon_size The size of the icon to get.
   * @param thumbnail_size The size of the thumbnail to get.
   * @param external_flags The external flags to get.
   * @param infos The screen source information to get.
   * @param count The number of screen source information to get.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int enum_screen_source_info(const traa_size icon_size, const traa_size thumbnail_size,
                                     const unsigned int external_flags,
                                     traa_screen_source_info **infos, int *count);

  /**
   * @brief Frees the screen source information.
   *
   * @param infos The screen source information to free.
   * @param count The number of screen source information to free.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int free_screen_source_info(traa_screen_source_info infos[], int count);

  /**
   * @brief Creates a snapshot of the specified source.
   *
   * @param source_id The ID of the source to create a snapshot of.
   * @param snapshot_size The size of the snapshot.
   * @param data A pointer to a pointer to the snapshot data.
   * @param data_size The size of the snapshot data.
   * @param actual_size The actual size of the snapshot data.
   *
   * @return traa_error::TRAA_ERROR_NONE if successful, otherwise an error code.
   */
  static int create_snapshot(const int64_t source_id, const traa_size snapshot_size, uint8_t **data,
                             int *data_size, traa_size *actual_size);

  /**
   * @brief Frees the snapshot data.
   *
   * @param data The snapshot data to free.
   */
  static void free_snapshot(uint8_t *data);
#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
};

} // namespace base
} // namespace traa

#endif // TRAA_BASE_DEVICES_SCREEN_ENUMERATOR_H_