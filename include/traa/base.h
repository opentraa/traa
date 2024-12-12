#ifndef TRAA_BASE_H
#define TRAA_BASE_H

#include <traa/error.h>

#if defined(__APPLE__)
// use TARGET_OS_IPHONE and TARGET_OS_MAC to determine the platform
#include <TargetConditionals.h>
#endif // __APPLE__

#include <stdbool.h>
#include <stdint.h>

#define TRAA_MAX_DEVICE_ID_LENGTH 256
#define TRAA_MAX_DEVICE_NAME_LENGTH 256

#define TRAA_FULLSCREEN_SCREEN_ID -1
#define TRAA_INVALID_SCREEN_ID -2

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
  int32_t width;
  int32_t height;

#if defined(__cplusplus)
  traa_size() : width(0), height(0) {}
  traa_size(int32_t width, int32_t height) : width(width), height(height) {}
#endif // defined(__cplusplus)
} traa_size;

/**
 * @struct traa_point
 * @brief Represents a point in a 2D space.
 *
 * The traa_point struct contains the x and y coordinates of a point in a 2D space.
 * It is used to represent the position of an object in a 2D space.
 */
typedef struct traa_point {
  int32_t x;
  int32_t y;

#if defined(__cplusplus)
  traa_point() : x(0), y(0) {}
  traa_point(int32_t x, int32_t y) : x(x), y(y) {}
#endif // defined(__cplusplus)
} traa_point;

/**
 * @struct traa_rect
 * @brief Represents a rectangle in a 2D space.
 *
 * The traa_rect struct contains the origin and size of a rectangle in a 2D space.
 * It is used to represent the position and dimensions of an object in a 2D space.
 */
typedef struct traa_rect {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;

#if defined(__cplusplus)
  traa_rect() : left(0), top(0), right(0), bottom(0) {}
  traa_rect(int32_t left, int32_t top, int32_t right, int32_t bottom)
      : left(left), top(top), right(right), bottom(bottom) {}
#endif // defined(__cplusplus)
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

#if defined(__cplusplus)
  traa_device_info()
      : id(), name(), type(TRAA_DEVICE_TYPE_UNKNOWN), slot(TRAA_DEVICE_SLOT_UNKNOWN),
        orientation(TRAA_DEVICE_ORIENTATION_UNKNOWN), state(TRAA_DEVICE_STATE_IDLE) {}
#endif // defined(__cplusplus)
} traa_device_info;

#if defined(_WIN32) ||                                                                             \
    (defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE &&                                   \
     (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)) ||                                         \
    defined(__linux__)

/**
 * @brief The screen source flags for TRAA.
 */
typedef enum traa_screen_source_flags {
  /**
   * @brief No flags.
   *
   * This is the default flag.
   */
  TRAA_SCREEN_SOURCE_FLAG_NONE = 0,

  /**
   * @brief Ignore the screen source.
   *
   * This flag indicates that the screen source should be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_IGNORE_SCREEN = 1 << 0,

  /**
   * @brief Ignore the window source.
   *
   * This flag indicates that the window source should be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_IGNORE_WINDOW = 1 << 1,

  /**
   * @brief Ignore the minimized source.
   *
   * This flag indicates that the minimized source should be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_IGNORE_MINIMIZED = 1 << 2,

  /**
   * @brief Do not ignore the untitled source.
   *
   * This flag indicates that the untitled source should not be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNTITLED = 1 << 3,

  /**
   * @brief Do not ignore the unresponsive source.
   *
   * This flag indicates that the unresponsive source should not be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_UNRESPONSIVE = 1 << 4,

  /**
   * @brief Ignore the current process source.
   *
   * This flag indicates that the current process source should be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_IGNORE_CURRENT_PROCESS_WINDOWS = 1 << 5,

  /**
   * @brief Do not ignore the tool window source.
   *
   * This flag indicates that the tool window source should not be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_NOT_IGNORE_TOOLWINDOW = 1 << 6,

  /**
   * @brief Ignore the no process path source.
   *
   * This flag indicates that the no process path source should be ignored.
   */
  TRAA_SCREEN_SOURCE_FLAG_IGNORE_NOPROCESS_PATH = 1 << 7,

  /**
   * @brief Do not skip system windows source.
   *
   * This flag indicates that the system windows source should not be skipped.
   */
  TRAA_SCREEN_SOURCE_FLAG_NOT_SKIP_SYSTEM_WINDOWS = 1 << 8,

  /**
   * @brief Do not skip zero layer windows source.
   *
   * This flag indicates that the zero layer windows source should not be skipped.
   */
  TRAA_SCREEN_SOURCE_FLAG_NOT_SKIP_ZERO_LAYER_WINDOWS = 1 << 9,

  /**
   * @brief All flags.
   *
   * This flag indicates all flags.
   */
  TRAA_SCREEN_SOURCE_FLAG_ALL = 0xFFFFFFFF
} traa_screen_source_flags;

/**
 * @brief The screen capturer id for TRAA.
 */
typedef enum traa_screen_capturer_id {
  /**
   * @brief Unknown screen capturer id.
   *
   * This is the unknown screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_UNKNOWN = 0,

  /**
   * @brief Windows GDI screen capturer id.
   *
   * This is the Windows GDI screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_WIN_GDI = 1,

  /**
   * @brief Windows DXGI screen capturer id.
   *
   * This is the Windows DXGI screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_WIN_DXGI = 2,

  /**
   * @brief Windows magnifier screen capturer id.
   *
   * This is the Windows magnifier screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_WIN_MAGNIFIER = 3,

  /**
   * @brief Windows WGC screen capturer id.
   *
   * This is the Windows WGC screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_WIN_WGC = 4,

  /**
   * @brief Windows DWM screen capturer id.
   *
   * This is the Windows DWM screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_WIN_MAX = 20,

  /**
   * @brief Linux X11 screen capturer id.
   *
   * This is the Linux X11 screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_LINUX_X11 = 21,

  /**
   * @brief Linux Wayland screen capturer id.
   *
   * This is the Linux Wayland screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_LINUX_WAYLAND = 22,

  /**
   * @brief Linux Screen Capture screen capturer id.
   *
   * This is the Linux Screen Capture screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_LINUX_MAX = 40,

  /**
   * @brief Mac screen capturer id.
   *
   * This is the Mac screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_MAC = 41,

  /**
   * @brief Mac AVFoundation screen capturer id.
   *
   * This is the Mac AVFoundation screen capturer id.
   */
  TRAA_SCREEN_CAPTURER_ID_MAC_MAX = 60,

  /**
   * @brief Screen capturer id max.
   *
   * This is the screen capturer id max.
   */
  TRAA_SCREEN_CAPTURER_ID_MAX = 100
} traa_screen_capturer_id;

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
  int64_t id;

  /**
   * @brief The screen id.
   *
   * Default is `TRAA_INVALID_SCREEN_ID`, only valid when current source is window.
   * Used to identify the screen that the window is on.
   */
  int64_t screen_id;

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
   * @brief Indicates whether the source is primary.
   *
   * This flag indicates whether the source is primary.
   */
  bool is_primary;

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
   * @brief The title of the source.
   *
   * This is the title of the source.
   */
  const char title[TRAA_MAX_DEVICE_NAME_LENGTH];

  /**
   * @brief The process path of the source.
   *
   * This is the process path of the source.
   */
  const char process_path[TRAA_MAX_DEVICE_NAME_LENGTH];

  /**
   * @brief The data for the source's icon.
   *
   * This is the data for the source's icon.
   */
  const uint8_t *icon_data;

  /**
   * @brief The data for the source's thumbnail.
   *
   * This is the data for the source's thumbnail.
   */
  const uint8_t *thumbnail_data;

#if defined(__cplusplus)
  traa_screen_source_info()
      : id(TRAA_INVALID_SCREEN_ID), screen_id(TRAA_INVALID_SCREEN_ID), is_window(false),
        is_minimized(false), is_maximized(false), is_primary(false), rect(), icon_size(),
        thumbnail_size(), title("\0"), process_path("\0"), icon_data(nullptr),
        thumbnail_data(nullptr) {}
#endif // defined(__cplusplus)
} traa_screen_source_info;
#endif // _WIN32 || (__APPLE__ && TARGET_OS_MAC && (!defined(TARGET_OS_VISION) ||
       // !TARGET_OS_VISION)) || __linux__

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
  int32_t max_size;

  /**
   * @brief The maximum number of log files.
   *
   * This is the maximum number of log files that are kept.
   */
  int32_t max_files;

  /**
   * @brief The log level for TRAA.
   *
   * This is the log level for the log messages, which defaults to `TRAA_LOG_LEVEL_INFO`.
   */
  traa_log_level level;

#if defined(__cplusplus)
  traa_log_config(const char *log_file = nullptr, int32_t max_size = 1024 * 1024 * 2,
                  int32_t max_files = 3, traa_log_level level = TRAA_LOG_LEVEL_INFO)
      : log_file(log_file), max_size(max_size), max_files(max_files), level(level) {}
#endif // defined(__cplusplus)
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

#if defined(__cplusplus)
  traa_event_handler() : on_error(nullptr), on_device_event(nullptr) {}
#endif // defined(__cplusplus)
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

#if defined(__cplusplus)
  traa_config() : userdata(nullptr) {}
#endif // defined(__cplusplus)
} traa_config;

#endif // TRAA_BASE_H