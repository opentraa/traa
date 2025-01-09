cmake_minimum_required(VERSION 3.10)

project(core)

# add subdirectories
add_subdirectory(base64)
add_subdirectory(devices)
add_subdirectory(folder)
add_subdirectory(numerics)
if(WIN32)
    add_subdirectory(strings)
endif()
add_subdirectory(system)
add_subdirectory(thread)
add_subdirectory(utils)

# core files
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

# set core library name
set(TRAA_LIBRARY_CORE core)

# add core library
add_library(${TRAA_LIBRARY_CORE} STATIC)

# alias core to traa:base::core
add_library(traa::base::core ALIAS ${TRAA_LIBRARY_CORE})

# add core files
target_sources(${TRAA_LIBRARY_CORE} PRIVATE
    $<TARGET_OBJECTS:traa::base::base64>
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
    target_sources(${TRAA_LIBRARY_CORE} PRIVATE  
        $<TARGET_OBJECTS:traa::base::strings>
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
elseif(ANDROID)
elseif(MACOS)
    target_sources(${TRAA_LIBRARY_CORE} PRIVATE
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
elseif(LINUX)
    target_sources(${TRAA_LIBRARY_CORE} PRIVATE
        $<TARGET_OBJECTS:traa::base::devices::screen>
    )
endif()

# set core folder
set_target_properties(${TRAA_LIBRARY_CORE} PROPERTIES FOLDER base)
