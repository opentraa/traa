#ifndef TRAA_BASE_H
#define TRAA_BASE_H

#include <traa/error.h>

/**
 * @brief The context for TRAA.
 *
 * This is the context for TRAA.
 */
typedef void *traa_context;

/**
 * @brief The userdata for TRAA.
 *
 * This is the userdata for TRAA.
 */
typedef void *traa_userdata;

/**
 * @brief Enumeration of device types used in the TRAA system.
 *
 * This enumeration defines the possible device types used in the TRAA system.
 */
typedef enum traa_device_type {
  TRAA_DEVICE_TYPE_UNKNOWN = 0,
  /**
   * @brief Camera device type.
   *
   * This is the camera device type.
   */
  TRAA_DEVICE_TYPE_CAMERA = 1,

  /**
   * @brief Microphone device type.
   *
   * This is the microphone device type.
   */
  TRAA_DEVICE_TYPE_MICROPHONE = 2,

  /**
   * @brief Speaker device type.
   *
   * This is the speaker device type.
   */
  TRAA_DEVICE_TYPE_SPEAKER = 3,

  /**
   * @brief Screen device type.
   *
   * This is the screen device type.
   */
  TRAA_DEVICE_TYPE_SCREEN = 4,

  /**
   * @brief Window device type.
   *
   * This is the window device type.
   */
  TRAA_DEVICE_TYPE_WINDOW = 5,

  /**
   * @brief Media file device type.
   *
   * This is the media file device type.
   */
  TRAA_DEVICE_TYPE_MEDIA_FILE = 6,
} traa_device_type;

/**
 * @brief Enumeration representing the device slots for TRAA devices.
 *
 * This enumeration defines the possible device slots for TRAA devices.
 * The slots include unknown, USB, Bluetooth, and network slots.
 *
 * @see traa_device_slot
 */
typedef enum traa_device_slot {
  /**
   * @brief Unknown device slot.
   *
   * This is the unknown device slot.
   */
  TRAA_DEVICE_SLOT_UNKNOWN = 0,

  /**
   * @brief USB device slot.
   *
   * This is the USB device slot.
   */
  TRAA_DEVICE_SLOT_USB = 1,

  /**
   * @brief Bluetooth device slot.
   *
   * This is the Bluetooth device slot.
   */
  TRAA_DEVICE_SLOT_BLUETOOTH = 2,

  /**
   * @brief Network device slot.
   *
   * This is the network device slot.
   */
  TRAA_DEVICE_SLOT_NETWORK = 3,
} traa_device_slot;

/**
 * @brief Enumeration representing the orientation of a TRAA device.
 *
 * The traa_device_orientation enumeration defines the possible orientations of a TRAA device.
 */
typedef enum traa_device_orientation {
  /**
   * @brief Unknown device orientation.
   *
   * This is the unknown device orientation.
   */
  TRAA_DEVICE_ORIENTATION_UNKNOWN = 0,

  /**
   * @brief Front device orientation.
   *
   * This is the front device orientation.
   */
  TRAA_DEVICE_ORIENTATION_FRONT = 1,

  /**
   * @brief Back device orientation.
   *
   * This is the back device orientation.
   */
  TRAA_DEVICE_ORIENTATION_BACK = 2,
} traa_device_orientation;

/**
 * @brief Enumeration representing the state of a TRAA device.
 *
 * The traa_device_state enumeration defines the possible states of a TRAA device.
 */
typedef enum traa_device_state {
  /**
   * @brief Idle device state.
   *
   * This is the idle device state.
   */
  TRAA_DEVICE_STATE_IDLE = 0,

  /**
   * @brief Posting device state.
   *
   * This is the posting device state.
   */
  TRAA_DEVICE_STATE_POSTING = 1,

  /**
   * @brief Active device state.
   *
   * This is the active device state.
   */
  TRAA_DEVICE_STATE_ACTIVE = 2,

  /**
   * @brief Paused device state.
   *
   * This is the paused device state.
   */
  TRAA_DEVICE_STATE_PAUSED = 3,
} traa_device_state;

/**
 * @brief Enumeration representing the event of a TRAA device.
 *
 * The traa_device_event enumeration defines the possible events of a TRAA device.
 */
typedef enum traa_device_event {
  TRAA_DEVICE_EVENT_UNKNOWN = 0,

  // operation
  TRAA_DEVICE_EVENT_ATTACHING = 1,
  TRAA_DEVICE_EVENT_ATTACHED = 2,
  TRAA_DEVICE_EVENT_DETACHING = 3,
  TRAA_DEVICE_EVENT_DETACHED = 4,

  // nework and bluetooth
  TRAA_DEVICE_EVENT_CONNECTING = 5,
  TRAA_DEVICE_EVENT_CONNECTED = 6,
  TRAA_DEVICE_EVENT_DISCONNECTING = 7,
  TRAA_DEVICE_EVENT_DISCONNECTED = 8,

  // usb
  TRAA_DEVICE_EVENT_PLUGGING = 9,
  TRAA_DEVICE_EVENT_PLUGGED = 10,
  TRAA_DEVICE_EVENT_UNPLUGGING = 11,
  TRAA_DEVICE_EVENT_UNPLUGGED = 12,

  // screen and window
  TRAA_DEVICE_EVENT_MINIMIZING = 13,
  TRAA_DEVICE_EVENT_MINIMIZED = 14,
  TRAA_DEVICE_EVENT_MAXIMIZING = 15,
  TRAA_DEVICE_EVENT_MAXIMIZED = 16,
  TRAA_DEVICE_EVENT_CLOSEING = 17,
  TRAA_DEVICE_EVENT_CLOSED = 18,
  TRAA_DEVICE_EVENT_RESIZING = 19,
  TRAA_DEVICE_EVENT_RESIZED = 20,

  // media file
  TRAA_DEVICE_EVENT_MAPPING = 21,
  TRAA_DEVICE_EVENT_MAPPED = 22,
  TRAA_DEVICE_EVENT_UNMAPPING = 23,
  TRAA_DEVICE_EVENT_UNMAPPED = 24,

  // properties
} traa_device_event;

/**
 * @brief The device info for TRAA.
 *
 * This is the device info for TRAA.
 */
typedef struct traa_device_info {
  /**
   * @brief The device id.
   *
   * This is the device id.
   */
  const char *id;

  /**
   * @brief The device name.
   *
   * This is the device name.
   */
  const char *name;

  /**
   * @brief The device type.
   *
   * This is the device type.
   */
  traa_device_type type;

  /**
   * @brief The device slot.
   *
   * This is the device slot.
   */
  traa_device_slot slot;

  /**
   * @brief The device orientation.
   *
   * This is the device orientation.
   */
  traa_device_orientation orientation;

  /**
   * @brief The device state.
   *
   * This is the device state.
   */
  traa_device_state state;

  traa_device_info(const char *id = nullptr, const char *name = nullptr,
                   traa_device_type type = TRAA_DEVICE_TYPE_UNKNOWN,
                   traa_device_slot slot = TRAA_DEVICE_SLOT_UNKNOWN,
                   traa_device_orientation orientation = TRAA_DEVICE_ORIENTATION_UNKNOWN,
                   traa_device_state state = TRAA_DEVICE_STATE_IDLE)
      : id(id), name(name), type(type), slot(slot), orientation(orientation), state(state) {}
} traa_device_info;

/**
 * @brief The log level for TRAA.
 *
 * This is the log level for TRAA.
 */
typedef enum traa_log_level {
  /**
   * @brief The trace log level.
   *
   * This is the trace log level.
   */
  TRAA_LOG_LEVEL_TRACE = 0,

  /**
   * @brief The debug log level.
   *
   * This is the debug log level.
   */
  TRAA_LOG_LEVEL_DEBUG = 1,

  /**
   * @brief The info log level.
   *
   * This is the info log level.
   */
  TRAA_LOG_LEVEL_INFO = 2,

  /**
   * @brief The warn log level.
   *
   * This is the warn log level.
   */
  TRAA_LOG_LEVEL_WARN = 3,

  /**
   * @brief The error log level.
   *
   * This is the error log level.
   */
  TRAA_LOG_LEVEL_ERROR = 4,

  /**
   * @brief The fatal log level.
   *
   * This is the fatal log level.
   */
  TRAA_LOG_LEVEL_FATAL = 5,

  /**
   * @brief The off log level.
   *
   * This is the off log level.
   */
  TRAA_LOG_LEVEL_OFF = 6
} traa_log_level;

/**
 * @brief Configuration options for TRAA logging.
 *
 * This struct defines the configuration options for TRAA logging. It specifies the log file,
 * maximum size of the log file, maximum number of log files, and log level.
 */
typedef struct traa_log_config {
  /**
   * @brief The log file for TRAA.
   *
   * This is the file where the log messages are written.
   * If this is set to `nullptr`, the log messages are written to the console by default, and other
   * log options are ignored when you call `traa_init` or `traa_set_log_config`.
   */
  const char *log_file;

  /**
   * @brief The maximum size of the log file.
   *
   * This is the maximum size of the log file in bytes.
   */
  int max_size;

  /**
   * @brief The maximum number of log files.
   *
   * This is the maximum number of log files that are kept.
   */
  int max_files;

  /**
   * @brief The log level for TRAA.
   *
   * This is the log level for the log messages, which defaults to `TRAA_LOG_LEVEL_INFO`.
   */
  traa_log_level level;

  traa_log_config(const char *log_file = nullptr, int max_size = 1024 * 1024 * 2, int max_files = 3,
                  traa_log_level level = TRAA_LOG_LEVEL_INFO)
      : log_file(log_file), max_size(max_size), max_files(max_files), level(level) {}
} traa_log_config;

/**
 * @brief Structure representing an event handler for TRAA.
 *
 * This structure defines a set of function pointers that can be used to handle TRAA events.
 */
typedef struct traa_event_handler {
  /**
   * @brief Function pointer for handling log messages.
   *
   * This function pointer is called when a log message is generated by TRAA.
   *
   * @param userdata The user data associated with the event handler.
   * @param level The log level of the message.
   * @param message The log message.
   */
  void (*on_error)(const traa_userdata userdata, traa_error error, const char *message);

  void (*on_device_event)(const traa_userdata userdata, const traa_device_info *device_info,
                          traa_device_event event);

  traa_event_handler() : on_error(nullptr), on_device_event(nullptr) {}
} traa_event_handler;

/**
 * @brief The configuration structure for the traa library.
 *
 * This structure holds the configuration options for the traa library.
 * It includes the userdata, log configuration, and event handler.
 */
typedef struct traa_config {
  /**
   * @brief The userdata associated with the traa library.
   *
   * This userdata is passed to the event handler functions.
   */
  traa_userdata userdata;

  /**
   * @brief The log configuration for the traa library.
   *
   * This configuration is used to set the log file, max size, max files, and log level.
   */
  traa_log_config log_config;

  /**
   * @brief The event handler for the traa library.
   *
   * This event handler is used to handle TRAA events.
   */
  traa_event_handler event_handler;

  traa_config() : userdata(nullptr) {}
} traa_config;

#endif // TRAA_BASE_H