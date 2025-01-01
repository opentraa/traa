cmake_minimum_required(VERSION 3.10)

# set utils files
set(TRAA_LIBRARY_BASE_UTILS_FILES
    "time_utils.h"
    "time_utils.cc"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_UTILS_FILES
        "win/create_direct3d_device.h"
        "win/create_direct3d_device.cc"
        "win/get_activation_factory.h"
        "win/get_activation_factory.cc"
        "win/hstring.h"
        "win/hstring.cc"
        "win/registry.h"
        "win/registry.cc"
        "win/scoped_com_initializer.h"
        "win/scoped_com_initializer.cc"
        "win/version.h"
        "win/version.cc"
    )
elseif(ANDROID)
elseif(APPLE)
elseif(LINUX)
endif()

# set source group  
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_UTILS_FILES})

# set utils library name
set(TRAA_LIBRARY_BASE_UTILS utils)

# add utils library
add_library(${TRAA_LIBRARY_BASE_UTILS} OBJECT)

# alias utils to traa:base:utils
add_library(traa::base::utils ALIAS ${TRAA_LIBRARY_BASE_UTILS})

# add utils files
target_sources(${TRAA_LIBRARY_BASE_UTILS} PRIVATE ${TRAA_LIBRARY_BASE_UTILS_FILES})

# set utils folder
set_target_properties(${TRAA_LIBRARY_BASE_UTILS} PROPERTIES FOLDER base)
