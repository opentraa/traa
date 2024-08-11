#ifndef TRAA_BASE_H
#define TRAA_BASE_H

#include <traa/error.h>

#define TRAA_MAX_DEVICE_ID_LENGTH 256
#define TRAA_MAX_DEVICE_NAME_LENGTH 256

#define TRAA_INVALID_SCREEN_ID -1
#define TRAA_FULLSCREEN_SCREEN_ID -2

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
 * @struct traa_size
 * @brief Represents the size of an object.
 *
 * The traa_size struct contains the width and height of an object.
 * It is used to represent the dimensions of an object in a 2D space.
 */
typedef struct traa_size {
  int width;
  int height;

  traa_size() : width(0), height(0) {}
} traa_size;

/**
 * @struct traa_point
 * @brief Represents a point in a 2D space.
 *
 * The traa_point struct contains the x and y coordinates of a point in a 2D space.
 * It is used to represent the position of an object in a 2D space.
 */
typedef struct traa_point {
  int x;
  int y;

  traa_point() : x(0), y(0) {}
} traa_point;

/**
 * @struct traa_rect
 * @brief Represents a rectangle in a 2D space.
 *
 * The traa_rect struct contains the origin and size of a rectangle in a 2D space.
 * It is used to represent the position and dimensions of an object in a 2D space.
 */
typedef struct traa_rect {
  int x;
  int y;
  int width;
  int height;

  traa_rect() : x(0), y(0), width(0), height(0) {}
} traa_rect;

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
   * @brief Media file device type.
   *
   * This is the media file device type.
   */
  TRAA_DEVICE_TYPE_MEDIA_FILE = 4,
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
  char id[TRAA_MAX_DEVICE_ID_LENGTH];

  /**
   * @brief The device name.
   *
   * This is the device name.
   */
  char name[TRAA_MAX_DEVICE_NAME_LENGTH];

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

  traa_device_info()
      : id(), name(), type(TRAA_DEVICE_TYPE_UNKNOWN), slot(TRAA_DEVICE_SLOT_UNKNOWN),
        orientation(TRAA_DEVICE_ORIENTATION_UNKNOWN), state(TRAA_DEVICE_STATE_IDLE) {}
} traa_device_info;

#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE) ||               \
    defined(__linux__)
/**
 * @brief The screen source info for TRAA.
 *
 * This is the screen source info for TRAA.
 */
typedef struct traa_screen_source_info {
  /**
   * @brief The screen source id.
   *
   * This is the screen source id. Default is `TRAA_INVALID_SCREEN_ID`.
   */
  unsigned int id;

  /**
   * @brief The screen id.
   *
   * Default is `TRAA_INVALID_SCREEN_ID`, only valid when current source is window.
   * Used to identify the screen that the window is on.
   */
  unsigned int screen_id;

  /**
   * @brief Indicates whether the source is a window or screen.
   *
   * This flag indicates whether the source is a window or screen.
   */
  bool is_window;

  /**
   * @brief Indicates whether the source is minimized.
   *
   * This flag indicates whether the source is minimized.
   */
  bool is_minimized;

  /**
   * @brief Indicates whether the source is maximized.
   *
   * This flag indicates whether the source is maximized.
   */
  bool is_maximized;

  /**
   * @brief Indicates whether the source is in fullscreen mode.
   *
   * This flag indicates whether the source is in fullscreen mode.
   */
  bool is_fullscreen;

  /**
   * @brief The position and size of the source.
   *
   * This is the position and size of the source on the full virtual screen.
   */
  traa_rect rect;

  /**
   * @brief The size of the source's icon.
   *
   * This is the size of the source's icon.
   */
  traa_size icon_size;

  /**
   * @brief The size of the source's thumbnail.
   *
   * This is the size of the source's thumbnail.
   */
  traa_size thumbnail_size;

  /**
   * @brief The data for the source's icon.
   *
   * This is the data for the source's icon.
   */
  const char *icon_data;

  /**
   * @brief The data for the source's thumbnail.
   *
   * This is the data for the source's thumbnail.
   */
  const char *thumbnail_data;

  traa_screen_source_info()
      : id(TRAA_INVALID_SCREEN_ID), screen_id(TRAA_INVALID_SCREEN_ID), is_window(false),
        is_minimized(false), is_maximized(false), is_fullscreen(false), rect(), icon_size(),
        thumbnail_size(), icon_data(nullptr), thumbnail_data(nullptr) {}
} traa_screen_source_info;
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && !TARGET_OS_IPHONE) || __linux__

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

  /**
   * @brief Function pointer for handling device events.
   *
   * This function pointer is called when a device event occurs in TRAA.
   *
   * @param userdata The user data associated with the event handler.
   * @param info The device information associated with the event.
   * @param event The event that occurred.
   */
  void (*on_device_event)(const traa_userdata userdata, const traa_device_info *info,
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