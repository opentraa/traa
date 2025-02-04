cmake_minimum_required(VERSION 3.10)

# set system files
set(TRAA_LIBRARY_BASE_SYSTEM_FILES
    "cpu_features_wrapper.h"
    "cpu_features.cc"
    "cpu_info.cc"
    "cpu_info.h"
    "metrics.cc"
    "metrics.h"
    "sleep.cc"
    "sleep.h"
    "system_time.cc"
    "system_time.h"
)
if(WIN32)
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_SYSTEM_FILES "cpu_features_android.cc")
elseif(APPLE)
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_SYSTEM_FILES "cpu_features_linux.cc")
endif()

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_SYSTEM_FILES})

# set system library name
set(TRAA_LIBRARY_BASE_SYSTEM system)

# add system library
add_library(${TRAA_LIBRARY_BASE_SYSTEM} OBJECT)

# alias system to traa:base:system
add_library(traa::base::system ALIAS ${TRAA_LIBRARY_BASE_SYSTEM})

# add system files
target_sources(${TRAA_LIBRARY_BASE_SYSTEM} PRIVATE ${TRAA_LIBRARY_BASE_SYSTEM_FILES})

# set system folder
set_target_properties(${TRAA_LIBRARY_BASE_SYSTEM} PROPERTIES FOLDER base)
