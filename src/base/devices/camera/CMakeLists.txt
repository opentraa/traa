cmake_minimum_required(VERSION 3.10)

set(TRAA_LIBRARY_BASE_DEVICES_CAMERA_FILES
    "video_capture_defines.h"
    "video_capture_defines.cc"
    "video_capture_config.h"
    "video_capture_impl.h"
    "video_capture_impl.cc"
    "device_info_impl.h"
    "device_info_impl.cc"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_CAMERA_FILES
        "win/help_functions_ds.h"
        "win/help_functions_ds.cc"
        "win/device_info_ds.h"
        "win/device_info_ds.cc"
        "win/sink_filter_ds.h"
        "win/sink_filter_ds.cc"
        "win/video_capture_ds.h"
        "win/video_capture_ds.cc"
        "win/video_capture_factory_windows.cc"
    )
endif()

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_DEVICES_CAMERA_FILES})

# set devices library name
set(TRAA_LIBRARY_BASE_DEVICES_CAMERA camera)

# add devices library
add_library(${TRAA_LIBRARY_BASE_DEVICES_CAMERA} OBJECT)

# alias devices to traa::base::devices::camera
add_library(traa::base::devices::camera ALIAS ${TRAA_LIBRARY_BASE_DEVICES_CAMERA})

# add devices files
target_sources(${TRAA_LIBRARY_BASE_DEVICES_CAMERA} PRIVATE ${TRAA_LIBRARY_BASE_DEVICES_CAMERA_FILES})

# set devices folder
set_target_properties(${TRAA_LIBRARY_BASE_DEVICES_CAMERA} PROPERTIES FOLDER base/devices)

if(WIN32)
    target_link_libraries(${TRAA_LIBRARY_BASE_DEVICES_CAMERA} PRIVATE strmiids ole32 oleaut32)
endif()
