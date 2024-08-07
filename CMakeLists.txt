# ##############################################################################
# CMake Section
# ##############################################################################

# Set minimum verison of cmake to 3.10.
cmake_minimum_required(VERSION 3.10)

# Recognize ``TEST`` as a operator for the ``if()`` command.
if(POLICY CMP0064)
    cmake_policy(SET CMP0064 NEW)
endif(POLICY CMP0064)

# ``MACOSX_RPATH`` is enabled by default.
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)

# Compiler id for Apple Clang is now ``AppleClang``
if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif(POLICY CMP0025)

# Import toolchain files.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/cmake")

# Use the ``FOLDER`` target property to organize targets into folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ##############################################################################
# Project Section
# ##############################################################################

# Name project traa.
project(traa)
message(STATUS "[TRAA] configuring...")

# Reuqire C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMAKE_SYSTEM_NAME only be set after project defined.
message(STATUS "[TRAA] current target system name ${CMAKE_SYSTEM_NAME}")
message(STATUS "[TRAA] current host system name ${CMAKE_HOST_SYSTEM_NAME}")

# Determine whether current host system is supported or not.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "[TRAA] current host system ${CMAKE_HOST_SYSTEM_NAME} is not supported to build for now")
endif()

# Preset flags for compiler.
# default set optimize level to O3
# macos O3
# windows O2
# android Oz
# ios Oz
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")

    # /wd is used to disable specific warning messages.
    # /wo is used to treat specific warning messages as errors.
    # /we is used to treat all warnings as errors.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4297")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MT /Zi")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi")
elseif(UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb -Wall -O3 -fPIC -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Werror=uninitialized -Werror=return-type -Wno-reorder -Wexceptions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -ggdb -Wall -O3 -fPIC -Wno-error=deprecated-declarations -Wno-deprecated-declarations -Werror=uninitialized -Werror=return-type -Wno-reorder -Wno-unused-function -Wexceptions")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()

if(MACOS)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.13")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
elseif(IOS)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Oz")
elseif(ANDROID)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Oz")
endif()

# Define output directory.
set(TRAA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/output")
set(TRAA_OUTPUT_DIR_REAL "${TRAA_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}")
message(STATUS "[TRAA] TRAA_OUTPUT_DIR: ${TRAA_OUTPUT_DIR}")
message(STATUS "[TRAA] TRAA_OUTPUT_DIR_REAL: ${TRAA_OUTPUT_DIR_REAL}")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(TRAA_DEBUG)
endif()

# ##############################################################################
# TRAA Library Section
# ##############################################################################

# public header files
set(TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR ${CMAKE_HOME_DIRECTORY}/include)
file(GLOB_RECURSE TRAA_LIBRARY_PUBLIC_HEADER_FILES ${TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR}/*.h)

# traa::base::folder
set(TRAA_LIBRARY_BASE_FOLDER_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder.cc"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder_win.cc")
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder_android.cc")
elseif(APPLE)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder_darwin.mm")
elseif(UNIX)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder_posix.cc")
endif()

# traa::base::log
set(TRAA_LIBRARY_BASE_LOG_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/log/logger.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/log/logger.cc"
)

# traa::base::strings
set(TRAA_LIBRARY_BASE_STRINGS_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/strings/string_trans.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/strings/string_trans.cc"
)

# traa::base::thread
set(TRAA_LIBRARY_BASE_THREAD_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/callback.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/task_queue.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util.h"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_win.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_win.cc")
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_posix.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_posix.cc")
elseif(APPLE)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_darwin.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_darwin.cc")
elseif(UNIX)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_posix.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_posix.cc")
endif()

# traa::base
set(TRAA_LIBRARY_BASE_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/arch.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/disallow.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/hedley.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/platform.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/singleton.h"
)

# traa::main::utils
set(TRAA_LIBRARY_MAIN_UTILS_FILES
    "${CMAKE_HOME_DIRECTORY}/src/main/utils/obj_string.h"
)

# traa::main
set(TRAA_LIBRARY_MAIN_FILES
    "${CMAKE_HOME_DIRECTORY}/src/main/engine.h"
    "${CMAKE_HOME_DIRECTORY}/src/main/engine.cc"
)

# thirdparty spdlog
set(TRAA_THIRDPARTY_SPDLOG_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/spdlog/include")
add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "thirdparty")

# thirdparty nlohmann::json
set(TRAA_THIRDPARTY_JSON_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/json/single_include")
set(TRAA_THIRDPARTY_JSON_FILE
    "${TRAA_THIRDPARTY_JSON_INCLUDE_DIR}/nlohmann/json.hpp"
    "${TRAA_THIRDPARTY_JSON_INCLUDE_DIR}/nlohmann/json_fwd.hpp"
)
source_group(TREE ${CMAKE_HOME_DIRECTORY} FILES ${TRAA_THIRDPARTY_JSON_FILE})

# TODO @sylar: do we really need absl?
# thirdparty absil-cpp
if(TRAA_ENABLE_ABSL)
    add_subdirectory(thirdparty/abseil-cpp)
    set(TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/abseil-cpp")
endif()

# thirdparty asio
set(TRAA_THIRDPARTY_ASIO_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/asio/asio/include")
file(GLOB_RECURSE TRAA_THIRDPARTY_ASIO_FILES
    ${CMAKE_HOME_DIRECTORY}/thirdparty/asio/asio/include/asio/*.hpp
)
source_group(TREE ${CMAKE_HOME_DIRECTORY} FILES ${TRAA_THIRDPARTY_ASIO_FILES})

# library include directories
set(TRAA_LIBRARY_INCLUDE_DIRS
    ${TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_SPDLOG_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_JSON_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_ASIO_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR}
    "${CMAKE_HOME_DIRECTORY}/src"
)

# library files
set(TRAA_LIBRARY_FILES
    # traa public header files
    ${TRAA_LIBRARY_PUBLIC_HEADER_FILES}

    # traa base files
    ${TRAA_LIBRARY_BASE_FOLDER_FILES}
    ${TRAA_LIBRARY_BASE_LOG_FILES}
    ${TRAA_LIBRARY_BASE_THREAD_FILES}
    ${TRAA_LIBRARY_BASE_FILES}

    # traa main files
    ${TRAA_LIBRARY_MAIN_UTILS_FILES}
    ${TRAA_LIBRARY_MAIN_FILES}

    # thirdparty files
    ${TRAA_THIRDPARTY_JSON_FILE}
    ${TRAA_THIRDPARTY_ASIO_FILES}

    # traa
    "${CMAKE_HOME_DIRECTORY}/src/traa.cc"
    "${CMAKE_HOME_DIRECTORY}/src/main.cc"
)
if(WIN32)
    list(APPEND TRAA_LIBRARY_FILES ${TRAA_LIBRARY_BASE_STRINGS_FILES})

    # source files
    configure_file(${CMAKE_HOME_DIRECTORY}/resources/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    list(APPEND TRAA_LIBRARY_FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()
source_group(TREE ${CMAKE_HOME_DIRECTORY} FILES ${TRAA_LIBRARY_FILES})

# add shared library
set(TRAA_LIBRARY traa)
set(TRAA_LIBRARY_NAME "${TRAA_LIBRARY}")
add_library(${TRAA_LIBRARY} SHARED ${TRAA_LIBRARY_FILES})

# add include directories
target_include_directories(${TRAA_LIBRARY} PUBLIC "$<BUILD_INTERFACE:${TRAA_LIBRARY_INCLUDE_DIRS}>")

# add compile definitions
target_compile_definitions(${TRAA_LIBRARY} PUBLIC TRAA_EXPORT)
set_target_properties(${TRAA_LIBRARY} PROPERTIES
    VERSION "${TRAA_VERSION_SHORT}"
    SOVERSION "${TRAA_VERSION_SHORT}"
    PUBLIC_HEADER "${TRAA_LIBRARY_PUBLIC_HEADER_FILES}"
    ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    OUTPUT_NAME "${TRAA_LIBRARY_NAME}"
)

# thirdparty link
if(TRAA_ENABLE_ABSL)
    target_link_libraries(${TRAA_LIBRARY} PRIVATE 
        absl::algorithm
        absl::base
        absl::debugging
        absl::flat_hash_map
        absl::flags
        absl::memory
        absl::meta
        absl::numeric
        absl::random_random
        absl::strings
        absl::synchronization
        absl::time
        absl::utility)
endif()

# thirdparty compile definitions
target_compile_definitions(${TRAA_LIBRARY} PRIVATE SPDLOG_NO_EXCEPTIONS)

# platform flags
if(APPLE)
    set(TRAA_LIBRARY_IDENTIFIER "io.traa.core")
    set(TRAA_LIBRARY_FRAMEWORKS)

    list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework Foundation")

    if(IOS)
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework UIKit")
    elseif(MACOS)
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework AppKit")
    endif()

    target_link_libraries(${TRAA_LIBRARY} PRIVATE "${TRAA_LIBRARY_FRAMEWORKS}")

    set_target_properties(${TRAA_LIBRARY} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER "${TRAA_LIBRARY_IDENTIFIER}"
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${TRAA_VERSION_SHORT}"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "${TRAA_VERSION_SHORT}"
        CXX_VISIBILITY_PRESET hidden
        LINK_FLAGS "-Wl ${TRAA_LIBRARY_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_LIBRARY} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
elseif(WIN32)
elseif(ANDROID)
    set_target_properties(${TRAA_LIBRARY} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
    )
    find_library(log-lib log)
    target_link_libraries(${TRAA_LIBRARY} PRIVATE ${log-lib})

    add_custom_command(
        TARGET ${TRAA_LIBRARY}
        POST_BUILD
        COMMAND cp -RP
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_OUTPUT_NAME}.so"
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_OUTPUT_NAME}-symbol.so"
    )
    add_custom_command(
        TARGET ${TRAA_LIBRARY}
        POST_BUILD
        COMMAND "${ANDROID_TOOLCHAIN_PREFIX}strip" --strip-all --verbose
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_OUTPUT_NAME}.so"
        COMMENT "Strip debug symbols done on final binary."
    )
elseif(UNIX)
    set_target_properties(${TRAA_LIBRARY} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LINK_FLAGS "-Wl,-rpath,./"
    )
endif()

# actions after build
if(NOT APPLE)
    set(TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR "${TRAA_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}/include")
    file(MAKE_DIRECTORY "${TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${TRAA_LIBRARY_PUBLIC_HEADER_FILES} "${TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR}"
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    )
endif()

# ##############################################################################
# TRAA Unittest Section
# ##############################################################################
if (NOT IOS AND NOT VISIONOS AND NOT ANDROID)
    add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/googletest)
    set_property(TARGET gtest PROPERTY FOLDER "thirdparty/googletest")
    set_property(TARGET gtest_main PROPERTY FOLDER "thirdparty/googletest")
    set_property(TARGET gmock PROPERTY FOLDER "thirdparty/googletest")
    set_property(TARGET gmock_main PROPERTY FOLDER "thirdparty/googletest")

    add_subdirectory(${CMAKE_HOME_DIRECTORY}/tests)
endif()

# ##############################################################################
# TRAA Console Section
# ##############################################################################
add_subdirectory(app)