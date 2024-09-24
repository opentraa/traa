#ifndef TRAA_TRAA_H
#define TRAA_TRAA_H

#include <traa/base.h>
#include <traa/export.h>

// TRAA is thread-safe. It is safe to call these functions from multiple threads.

/**
 * @brief The configuration for TRAA.
 *
 * This is the configuration for TRAA.
 *
 * @param config The configuration for TRAA.
 */
TRAA_API int TRAA_CALL traa_init(const traa_config *config);

/**
 * @brief The release for TRAA.
 *
 * This is the release for TRAA, which releases all resources and cleans up all internal state, all
 * unfinished tasks will be canceled.
 */
TRAA_API void TRAA_CALL traa_release();

/**
 * @brief Sets the event handler for the TRAA library.
 *
 * This function allows you to set the event handler for the TRAA library.
 * The event handler is used to handle TRAA events, such as log messages.
 *
 * @param handler A pointer to a `traa_event_handler` struct that contains the event handler
 * function pointers.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_set_event_handler(const traa_event_handler *handler);

/**
 * @brief The set log level for TRAA.
 *
 * This is the set log level for TRAA, which is default to TRAA_LOG_LEVEL_INFO.
 * This is stateless and can be called at any time.
 *
 * @param level The log level for TRAA.
 */
TRAA_API void TRAA_CALL traa_set_log_level(traa_log_level level);

/**
 * @brief Sets the log configuration for the TRAA library.
 *
 * This function allows you to set the log configuration for the TRAA library.
 * The log configuration specifies the log level, log file path, and other
 * log-related settings.
 *
 * @param config A pointer to a `traa_log_config` struct that contains
 * the log configuration settings.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_set_log(const traa_log_config *config);

/**
 * @brief Enumerates the devices of the specified type.
 *
 * This function enumerates the devices of the specified type and returns the device information.
 *
 * @param type The type of devices to enumerate.
 * @param infos A pointer to an array of traa_device_info structures to store the device
 * information.
 * @param count A pointer to an integer to store the number of devices found.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_enum_device_info(traa_device_type type, traa_device_info **infos,
                                             int *count);

/**
 * @brief Frees the memory allocated for the device information.
 *
 * This function frees the memory allocated for the device information.
 *
 * @param infos A pointer to an array of traa_device_info structures to free.
 * @return An integer value indicating the success or failure of the operation.
 *
 * @note This function must be called to free the memory allocated for the device information.
 */
TRAA_API int TRAA_CALL traa_free_device_info(traa_device_info infos[]);

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
/**
 * @brief Enumerates the screen sources.
 *
 * This function enumerates the screen sources and returns the screen source information.
 *
 * @param icon_size The size of the icon.
 * @param thumbnail_size The size of the thumbnail.
 * @param external_flags The external flags. See traa_screen_source_flags for more information. Can
 * be one or multiple flags.Recommended flags: TRAA_SCREEN_SOURCE_FLAG_IGNORE_UNTITLED |
 * TRAA_SCREEN_SOURCE_FLAG_IGNORE_UNRESPONSIVE | TRAA_SCREEN_SOURCE_FLAG_IGNORE_TOOLWINDOW,
 * @param infos A pointer to an array of traa_screen_source_info structures to store the screen
 * source information.
 * @param count A pointer to an integer to store the number of screen sources found.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_enum_screen_source_info(const traa_size icon_size,
                                                    const traa_size thumbnail_size,
                                                    const unsigned int external_flags,
                                                    traa_screen_source_info **infos, int *count);

/**
 * @brief Frees the memory allocated for the screen source information.
 *
 * This function frees the memory allocated for the screen source information.
 *
 * @param infos A pointer to an array of traa_screen_source_info structures to free.
 * @param count The number of screen sources.
 * @return An integer value indicating the success or failure of the operation.
 *
 * @note This function must be called to free the memory allocated for the screen source
 * information.
 */
TRAA_API int TRAA_CALL traa_free_screen_source_info(traa_screen_source_info infos[], int count);
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

#endif // TRAA_TRAA_H