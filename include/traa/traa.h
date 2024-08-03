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
 * @param event_handler A pointer to a `traa_event_handler` struct that contains
 *                      the event handler function pointers.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_set_event_handler(const traa_event_handler *event_handler);

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
 * @param log_config A pointer to a `traa_log_config` struct that contains
 *                   the log configuration settings.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_set_log(const traa_log_config *log_config);

#endif // TRAA_TRAA_H