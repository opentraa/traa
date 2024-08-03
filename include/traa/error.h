#ifndef TRAA_ERROR_H
#define TRAA_ERROR_H

/**
 * @brief The error codes for TRAA.
 *
 * This is the error codes for TRAA.
 */
typedef enum traa_error {
  /**
   * @brief No error.
   *
   * This is no error.
   */
  TRAA_ERROR_NONE = 0,

  /**
   * @brief Unknown error.
   *
   * This is an unknown error.
   */
  TRAA_ERROR_UNKNOWN = 1,

  /**
   * @brief Invalid argument error.
   *
   * This is an invalid argument error.
   */
  TRAA_ERROR_INVALID_ARGUMENT = 2,

  /**
   * @brief Invalid state error.
   *
   * This is an invalid state error.
   */
  TRAA_ERROR_INVALID_STATE = 3,

  /**
   * @brief Not implemented error.
   *
   * This is a not implemented error.
   */
  TRAA_ERROR_NOT_IMPLEMENTED = 4,

  /**
   * @brief Not supported error.
   *
   * This is a not supported error.
   */
  TRAA_ERROR_NOT_SUPPORTED = 5,

  /**
   * @brief Out of memory error.
   *
   * This is an out of memory error.
   */
  TRAA_ERROR_OUT_OF_MEMORY = 6,

  /**
   * @brief Out of range error.
   *
   * This is an out of range error.
   */
  TRAA_ERROR_OUT_OF_RANGE = 7,

  /**
   * @brief Permission denied error.
   *
   * This is a permission denied error.
   */
  TRAA_ERROR_PERMISSION_DENIED = 8,

  /**
   * @brief Resource busy error.
   *
   * This is a resource busy error.
   */
  TRAA_ERROR_RESOURCE_BUSY = 9,

  /**
   * @brief Resource exhausted error.
   *
   * This is a resource exhausted error.
   */
  TRAA_ERROR_RESOURCE_EXHAUSTED = 10,

  /**
   * @brief Resource unavailable error.
   *
   * This is a resource unavailable error.
   */
  TRAA_ERROR_RESOURCE_UNAVAILABLE = 11,

  /**
   * @brief Timed out error.
   *
   * This is a timed out error.
   */
  TRAA_ERROR_TIMED_OUT = 12,

  /**
   * @brief Too many requests error.
   *
   * This is too many requests error.
   */
  TRAA_ERROR_TOO_MANY_REQUESTS = 13,

  /**
   * @brief Unavailable error.
   *
   * This is an unavailable error.
   */
  TRAA_ERROR_UNAVAILABLE = 14,

  /**
   * @brief Unauthorized error.
   *
   * This is an unauthorized error.
   */
  TRAA_ERROR_UNAUTHORIZED = 15,

  /**
   * @brief Unsupported media type error.
   *
   * This is an unsupported media type error.
   */
  TRAA_ERROR_UNSUPPORTED_MEDIA_TYPE = 16,

  /**
   * @brief Already exists error.
   *
   * This is an already exists error.
   */
  TRAA_ERROR_ALREADY_EXISTS = 17,

  /**
   * @brief Not found error.
   *
   * This is a not found error.
   */
  TRAA_ERROR_NOT_FOUND = 18,

  /**
   * @brief Error count.
   *
   * This is the error count.
   */
  TRAA_ERROR_COUNT
} traa_error;

#endif // TRAA_ERROR_H