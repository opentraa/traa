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

/**
 * @brief Queries the capture capabilities of a specified camera device.
 *
 * This function retrieves all supported capture capabilities (resolution, frame rate, format) for
 * the camera device identified by @p device_id. The caller must free the returned array by calling
 * @ref traa_free_camera_capability.
 *
 * @param device_id The unique identifier of the camera device to query.
 * @param capabilities A pointer to receive the allocated array of traa_video_capability structures.
 * @param count A pointer to receive the number of capabilities in the returned array.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @note The caller is responsible for freeing the returned capabilities array by calling
 *       @ref traa_free_camera_capability.
 */
TRAA_API int TRAA_CALL traa_get_camera_capability(const char *device_id,
                                                   traa_video_capability **capabilities,
                                                   int *count);

/**
 * @brief Frees the memory allocated for camera capabilities.
 *
 * This function frees the memory allocated by @ref traa_get_camera_capability.
 *
 * @param capabilities A pointer to the traa_video_capability array to free.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @note This function must be called to free the memory allocated by
 *       @ref traa_get_camera_capability.
 */
TRAA_API int TRAA_CALL traa_free_camera_capability(traa_video_capability *capabilities);

/**
 * @brief Starts video capture on the specified camera device.
 *
 * This function starts capturing video frames from the camera device specified in @p config.
 * Captured frames are delivered through the @c on_video_frame callback registered in the
 * configuration. The callback is invoked on the capture thread, not the main task queue thread.
 *
 * @param config A pointer to a traa_camera_config structure containing the device ID, desired
 *               capture capability, frame callback, and user data.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @note The @c on_video_frame callback is called on the capture thread. The @c data pointer in
 *       traa_video_frame is only valid during the callback invocation.
 * @note Only one capture session per device is allowed. Starting capture on a device that is
 *       already capturing will return @c TRAA_ERROR_ALREADY_EXISTS.
 *
 * @see traa_stop_camera_capture
 * @see traa_camera_config
 */
TRAA_API int TRAA_CALL traa_start_camera_capture(const traa_camera_config *config);

/**
 * @brief Stops video capture on the specified camera device.
 *
 * This function stops an ongoing video capture session for the camera device identified by
 * @p device_id and releases the associated resources. After this call, the @c on_video_frame
 * callback registered for this device will no longer be invoked.
 *
 * @param device_id The unique identifier of the camera device to stop capturing.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @note If the specified device has no active capture session, this function returns
 *       @c TRAA_ERROR_NOT_FOUND.
 *
 * @see traa_start_camera_capture
 */
TRAA_API int TRAA_CALL traa_stop_camera_capture(const char *device_id);

#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&      \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&                                           \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
/**
 * @brief Enumerates the screen sources.
 *
 * This function enumerates the screen sources and returns the screen source information.
 *
 * @param icon_size The size of the icon.
 * @param thumbnail_size The size of the thumbnail.
 * @param external_flags The external flags. See traa_screen_source_flags for more information
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

/**
 * @brief Creates a snapshot of the specified source.
 *
 * This function creates a snapshot of the specified source and returns the snapshot data.
 *
 * @param source_id The ID of the source to create a snapshot of.
 * @param snapshot_size The size of the snapshot.
 * @param data A pointer to a pointer to the snapshot data.
 * @param data_size The size of the snapshot data.
 * @param actual_size The actual size of the snapshot data.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 */
TRAA_API int TRAA_CALL traa_create_snapshot(const int64_t source_id, const traa_size snapshot_size,
                                            uint8_t **data, int *data_size, traa_size *actual_size);

/**
 * @brief Frees the memory allocated for the snapshot data.
 *
 * This function frees the memory allocated for the snapshot data.
 *
 * @param data A pointer to the snapshot data to free.
 */
TRAA_API void TRAA_CALL traa_free_snapshot(uint8_t *data);

/**
 * @brief Starts continuous screen capture for the specified source.
 *
 * This function starts capturing screen frames from the source specified in @p config.
 * Captured frames are delivered through the @c on_video_frame callback registered in the
 * configuration. The callback is invoked on the capture thread with BGRA frame data, not the main
 * task queue thread.
 *
 * @param config A pointer to a traa_screen_capture_config structure containing the source ID,
 *               desired frame size, frame callback, and user data.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @note The @c on_video_frame callback is called on the capture thread. The @c data pointer in
 *       traa_video_frame is only valid during the callback invocation.
 * @note Only one capture session per source is allowed. Starting capture on a source that is
 *       already being captured will return @c TRAA_ERROR_ALREADY_EXISTS.
 *
 * @retval TRAA_ERROR_NONE on success.
 * @retval TRAA_ERROR_INVALID_ARGUMENT if @p config is nullptr, @c on_video_frame is nullptr,
 *         or @c source_id is @c TRAA_INVALID_SCREEN_ID.
 * @retval TRAA_ERROR_ALREADY_EXISTS if capture is already active for the given source_id.
 * @retval TRAA_ERROR_NOT_INITIALIZED if @c traa_init has not been called.
 *
 * @see traa_stop_screen_capture
 * @see traa_screen_capture_config
 */
TRAA_API int TRAA_CALL traa_start_screen_capture(const traa_screen_capture_config *config);

/**
 * @brief Stops screen capture for the specified source.
 *
 * This function stops an ongoing screen capture session for the source identified by @p source_id
 * and releases the associated resources. After this call, the @c on_video_frame callback registered
 * for this source will no longer be invoked.
 *
 * @param source_id The unique identifier of the screen source to stop capturing.
 * @return An integer value indicating the success or failure of the operation.
 *         A return value of 0 indicates success, while a non-zero value
 *         indicates failure.
 *
 * @retval TRAA_ERROR_NONE on success.
 * @retval TRAA_ERROR_INVALID_ARGUMENT if @p source_id is @c TRAA_INVALID_SCREEN_ID.
 * @retval TRAA_ERROR_NOT_FOUND if the specified source was not being captured.
 * @retval TRAA_ERROR_NOT_INITIALIZED if @c traa_init has not been called.
 *
 * @see traa_start_screen_capture
 */
TRAA_API int TRAA_CALL traa_stop_screen_capture(const int64_t source_id);
#endif // (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) &&
       // (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) &&
       // (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)

#endif // TRAA_TRAA_H