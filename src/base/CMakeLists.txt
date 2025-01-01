cmake_minimum_required(VERSION 3.10)

project(base)

# add subdirectories
add_subdirectory(devices)
add_subdirectory(folder)
add_subdirectory(numerics)
if(WIN32)
    add_subdirectory(strings)
endif()
add_subdirectory(system)
add_subdirectory(thread)
add_subdirectory(utils)

# base files
set(TRAA_LIBRARY_BASE_FILES
    "arch.h"
    "arraysize.h"
    "checks.h"
    "checks.cc"
    "disallow.h"
    "function_view.h"
    "hedley.h"
    "logger.cc"
    "logger.h"
    "platform.h"
    "random.h"
    "random.cc"
    "singleton.h"
    "thread_annotations.h"
    "type_traits.h"
    "win32.h"
)

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_FILES})

# set base library name
set(TRAA_LIBRARY_BASE base)

# add base library
add_library(${TRAA_LIBRARY_BASE} STATIC)

# alias base to traa:base
add_library(traa::base ALIAS ${TRAA_LIBRARY_BASE})

# add base files
target_sources(${TRAA_LIBRARY_BASE} PRIVATE
    $<TARGET_OBJECTS:traa::base::folder> 
    $<TARGET_OBJECTS:traa::base::numerics> 
    $<TARGET_OBJECTS:traa::base::system> 
    $<TARGET_OBJECTS:traa::base::thread> 
    $<TARGET_OBJECTS:traa::base::utils>
    $<TARGET_OBJECTS:traa::base::json>
    $<TARGET_OBJECTS:traa::base::asio>
    ${TRAA_LIBRARY_BASE_FILES}
)

if(WIN32)
    target_sources(${TRAA_LIBRARY_BASE} PRIVATE  
        $<TARGET_OBJECTS:traa::base::strings>
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
elseif(ANDROID)
elseif(MACOS)
    target_sources(${TRAA_LIBRARY_BASE} PRIVATE
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
elseif(LINUX)
    target_sources(${TRAA_LIBRARY_BASE} PRIVATE
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
endif()



