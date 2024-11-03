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
project(traa C CXX)
message(STATUS "[TRAA] configuring...")

# CMAKE_SYSTEM_NAME only be set after project defined.
message(STATUS "[TRAA] current target system name ${CMAKE_SYSTEM_NAME}")
message(STATUS "[TRAA] current host system name ${CMAKE_HOST_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
endif()

# Determine whether current host system is supported or not.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "[TRAA] current host system ${CMAKE_HOST_SYSTEM_NAME} is not supported to build for now")
endif()

# Reuqire C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
if(LINUX)
    # To enable x11 you have install x11 and x11-xext and x11-xcomposite development packages.
    # sudo apt-get install libx11-dev libxext-dev libxcomposite-dev
    option(TRAA_OPTION_ENABLE_X11 "Enable X11 support" ON)

    # TODO(@sylar) COPY FINDERS FROM OBS PROJECT AND USE list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/cmake/finders") TO INCLUDE THEM
    option(TRAA_OPTION_ENABLE_WAYLAND "Enable Wayland support" ON)
endif()

# Disable exception and rtti.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Use the highest warning level for Visual Studio.
    set(CMAKE_CXX_WARNING_LEVEL 4)

    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

    # Disable C++ exceptions.
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
    add_definitions(-D_HAS_EXCEPTIONS=0)

    # Disable RTTI.
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else()
    # Use -Wall for clang and gcc.
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

    # Use -Wextra for clang and gcc.
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

    # Use -Werror for clang and gcc.
    if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")

    # Disable C++ exceptions.
    string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

    # Disable RTTI.
    string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Preset flags for compiler.
# default set optimize level to O3
# macos O3
# windows O2
# android Oz
# ios Oz
if(WIN32)
    # https://learn.microsoft.com/en-us/cpp/build/reference/debug-generate-debug-info?view=msvc-170
    # The /DEBUG option puts the debugging information from linked object and library files
    # into a program database (PDB) file. It updates the PDB during subsequent builds of the program.
    #
    # An executable (an EXE or DLL file) created for debugging contains the name and path of the corresponding PDB.
    # The debugger reads the embedded name and uses the PDB when you debug the program.
    # The linker uses the base name of the program and the extension .pdb to name the program database,
    # and embeds the path where it was created. To override this default, set the /PDB option and specify a different file name.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")

    # /MTx is used to link the static library runtime, which is required by the official sdk.
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

    # /wd is used to disable specific warning messages.
    # /wo is used to treat specific warning messages as errors.
    # /we is used to treat all warnings as errors.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        /Zi \
        /O2 \
        /wo4018 \
        /wo4251 \
        /wo4275 \
        /we4033 \
        /we4700 \
        /wd26812 \
        /wd4996 \
        /wd4819 \
        /wd26495 \
        /wd4275 \
        /wd26439 \
        /wd26498 \
        /wd6285 \
        /wd26451 \
        /wd28020 \
        /wd26437 \
        /wd4297 \
        /wd4100 \
        /wd4505 \
        /wd4127 \
        /wd4702")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -g \
        -std=c++17 \
        -ggdb \
        -Wall \
        -fPIC \
        -fno-omit-frame-pointer \
        -fmerge-all-constants \
        -fno-strict-aliasing \
        -Wno-unknown-warning-option \
        -Wno-error=deprecated-declarations \
        -Wno-deprecated-declarations \
        -Werror=uninitialized \
        -Werror=return-type \
        -Wno-reorder \
        -Wno-unused-function \
        -Wno-unused-parameter \
        -Wno-error=unused-but-set-variable \
        -Wno-error=unused-const-variable")

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=shorten-64-to-32")
    endif()

    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
    endif()

    # Set visibility hidden for all symbols by default.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")

    # Hide symbols from the other static libraries
    if(APPLE)
    # The macOS ld linker allows you to hide symbols from a specific static library,
    # via -hidden-l instead of -l. Here's a quote from man ld on Mac:
    #
    # -hidden-lx  This is the same as the -lx for locating a static library,
    # but treats all global symbols from the static library as if
    # they are visibility hidden.  Useful when building a dynamic
    # library that uses a static library but does not want to
    # export anything from that static library.
    # set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} \
    # -Wl,-hidden-lxxxx")
    #
    # Do not use `-Wl,-hidden-lxxx`, it will hide all symbols from xxx.a static library.
    # Depending on https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/DynamicLibraryDesignGuidelines.html
    elseif(ANDROID)
        # Hide symbols from libc++ and libc++abi
        # see https://developer.android.com/ndk/guides/middleware-vendors#using_the_stl
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} \
            -Wl,--exclude-libs,libc++_static.a \
            -Wl,--exclude-libs,libc++abi.a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} \
                -Wl,--gc-sections \
                -Wl,-z,noexecstack \
                -Wl,-z,now \
                -Wl,-z,relro \
                -Wl,-z,defs \
                -Wl,--as-needed \
                -Wl,--no-undefined \
                -Wl,--build-id=sha1 -lc -ldl -landroid")

        #
        # Do not use -Wl,--exclude-libs,ALL, it will exclude all symbols from all static libraries,
        # We need to export jni interfaces from libxxxx.a
        # Instead, we choose to use `-Wl,--version-script=${CMAKE_HOME_DIRECTORY}/cmake/symbols.sym`
        # set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} \
        # -Wl,--exclude-libs,ALL")
    endif()
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

message(STATUS "[TRAA] ${CMAKE_CXX_FLAGS}")

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

# traa::base::devices
if(WIN32 OR MACOS OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TRAA_LIBRARY_BASE_DEVICES_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_frame.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_frame.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_geometry.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_geometry.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_region.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_region.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/enumerator.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/enumerator.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/mouse_cursor.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/mouse_cursor.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/shared_memory.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/shared_memory.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/utils.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/utils.cc"
    )
endif()

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/capture_utils.h")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/capture_utils.cc")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/cursor.h")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/cursor.cc")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/enumerator_win.cc")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/scoped_object_gdi.h")
elseif(APPLE AND NOT IOS AND NOT VISIONOS)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/darwin/desktop_configuration.mm")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/darwin/desktop_configuration.mm")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/darwin/enumerator_darwin.mm")
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/capture_utils.h")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/capture_utils.cc")
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/enumerator_linux.cc")
    # Enable x11 if TRAA_OPTION_ENABLE_X11 is set and X11 is found
    if(TRAA_OPTION_ENABLE_X11)
        find_package(X11)
        if(X11_FOUND AND X11_Xext_FOUND AND X11_Xcomposite_FOUND AND X11_Xrandr_FOUND)
            set(TRAA_OPTION_ENABLE_X11 ON)
            add_definitions(-DTRAA_ENABLE_X11)
            message(STATUS "[TRAA] X11 found")
            message(STATUS "[TRAA] X11_INCLUDE_DIR: ${X11_INCLUDE_DIR}")
            message(STATUS "[TRAA] X11_X11_LIB: ${X11_X11_LIB}")
            message(STATUS "[TRAA] X11_Xext_INCLUDE_PATH: ${X11_Xext_INCLUDE_PATH}")
            message(STATUS "[TRAA] X11_Xext_LIB: ${X11_Xext_LIB}")
            message(STATUS "[TRAA] X11_Xcomposite_INCLUDE_PATH: ${X11_Xcomposite_INCLUDE_PATH}")
            message(STATUS "[TRAA] X11_Xcomposite_LIB: ${X11_Xcomposite_LIB}")
            message(STATUS "[TRAA] X11_Xrandr_INCLUDE_PATH: ${X11_Xrandr_INCLUDE_PATH}")
            message(STATUS "[TRAA] X11_Xrandr_LIB: ${X11_Xrandr_LIB}")

            set(TRAA_X11_INCLUDE_DIRS 
                ${X11_INCLUDE_DIR} 
                ${X11_Xext_INCLUDE_PATH} 
                ${X11_Xcomposite_INCLUDE_PATH} 
                ${X11_Xrandr_INCLUDE_PATH})
            set(TRAA_X11_LIBS 
                ${X11_X11_LIB} 
                ${X11_Xext_LIB} 
                ${X11_Xcomposite_LIB} 
                ${X11_Xrandr_LIB})
            
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_atom_cache.h")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_atom_cache.cc")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_error_trap.h")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_error_trap.cc")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_server_pixel_buffer.h")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_server_pixel_buffer.cc")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_window_list_utils.h")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_window_list_utils.cc")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_window_property.cc")
            list(APPEND TRAA_LIBRARY_BASE_DEVICES_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/linux/x11/x_window_property.h")
        else()
            message(WARNING "[TRAA] X11 not found, disable X11 support")
            set(TRAA_OPTION_ENABLE_X11 OFF)
        endif()
    endif()
    # Enable wayland if TRAA_OPTION_ENABLE_WAYLAND is set
    if(TRAA_OPTION_ENABLE_WAYLAND)
        add_definitions(-DTRAA_ENABLE_WAYLAND)
    endif()
endif()

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
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "${CMAKE_HOME_DIRECTORY}/src/base/folder/folder_linux.cc")
endif()

# traa::base::log
set(TRAA_LIBRARY_BASE_LOG_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/log/logger.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/log/logger.cc"
)

# traa::base::os
if(WIN32)
    set(TRAA_LIBRARY_BASE_OS_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/utils/win/registry.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/utils/win/registry.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/utils/win/version.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/utils/win/version.cc"
    )
elseif(ANDROID)
elseif(APPLE)
elseif(LINUX)
endif()

# traa::base::strings
set(TRAA_LIBRARY_BASE_STRINGS_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/strings/string_trans.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/strings/string_trans.cc"
)

# traa::base::thread
set(TRAA_LIBRARY_BASE_THREAD_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/callback.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/ffuture.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/ffuture.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/task_queue.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/waitable_future.h"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_win.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_win.cc")
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_linux.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_linux.cc")
elseif(APPLE)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_darwin.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_darwin.cc")
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_linux.cc")
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_linux.cc")
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
add_definitions(-DSPDLOG_NO_EXCEPTIONS=ON)
add_definitions(-DFMT_EXCEPTIONS=OFF)
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

# thirdparty libyuv-static
add_definitions(-DLIBYUV_ENABLE_NEON=OFF) # TODO @sylar: how to enable neon?
add_definitions(-DLIBYUV_ENABLE_SVE=OFF) # TODO @sylar: how to enable sve?
add_definitions(-DLIBYUV_ENABLE_SME=OFF) # TODO @sylar: how to enable sme?
add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/libyuv-static)
set(TRAA_THIRDPARTY_LIBYUV_INCLUDE_DIR "${LIBYUV_INCLUDE_DIR}")
set_property(TARGET yuv PROPERTY FOLDER "thirdparty/libyuv")

# TODO @sylar: do we really need absl?
# thirdparty absil-cpp
if(TRAA_ENABLE_ABSL)
    add_subdirectory(thirdparty/abseil-cpp)
    set(TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/abseil-cpp")
endif()

# thirdparty asio
add_compile_definitions(ASIO_NO_EXCEPTIONS)
add_compile_definitions(ASIO_NO_TYPEID)
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
    ${TRAA_THIRDPARTY_LIBYUV_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_ASIO_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR}
    "${CMAKE_HOME_DIRECTORY}/src"
)

# library files
set(TRAA_LIBRARY_FILES

    # traa public header files
    ${TRAA_LIBRARY_PUBLIC_HEADER_FILES}

    # traa base files
    ${TRAA_LIBRARY_BASE_DEVICES_FILES}
    ${TRAA_LIBRARY_BASE_FOLDER_FILES}
    ${TRAA_LIBRARY_BASE_LOG_FILES}
    ${TRAA_LIBRARY_BASE_OS_FILES}
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
    configure_file(${CMAKE_HOME_DIRECTORY}/resources/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)
    list(APPEND TRAA_LIBRARY_FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

source_group(TREE ${CMAKE_HOME_DIRECTORY} FILES ${TRAA_LIBRARY_FILES})

# add shared library
set(TRAA_LIBRARY traa)
set(TRAA_LIBRARY_NAME "${TRAA_LIBRARY}")
add_library(${TRAA_LIBRARY} SHARED ${TRAA_LIBRARY_FILES})

# add include directories
target_include_directories(${TRAA_LIBRARY} PUBLIC "$<BUILD_INTERFACE:${TRAA_LIBRARY_INCLUDE_DIRS}>")

# add extra libraries libyuv
target_link_libraries(${TRAA_LIBRARY} PRIVATE yuv)

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
        LINK_FLAGS "-Wl ${TRAA_LIBRARY_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_LIBRARY} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET ${TRAA_LIBRARY}
            POST_BUILD
            COMMAND strip -r -S -x
            "${TRAA_OUTPUT_DIR_REAL}/${TRAA_LIBRARY_NAME}.framework/${TRAA_LIBRARY_NAME}"
            COMMENT "Strip debug and local symbols done on final binary."
        )
    endif()
elseif(WIN32)
    target_link_libraries(${TRAA_LIBRARY} PRIVATE
        dwmapi.lib
        Shcore.lib
    )
elseif(ANDROID)
    set_target_properties(${TRAA_LIBRARY} PROPERTIES
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
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_NAME}.so"
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_NAME}-symbol.so"
    )
    add_custom_command(
        TARGET ${TRAA_LIBRARY}
        POST_BUILD
        COMMAND "${ANDROID_TOOLCHAIN_ROOT}/bin/llvm-strip" --strip-all
        "${TRAA_OUTPUT_DIR_REAL}/lib${TRAA_LIBRARY_NAME}.so"
        COMMENT "Strip debug symbols done on final binary."
    )
elseif(LINUX)
    set_target_properties(${TRAA_LIBRARY} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LINK_FLAGS "-Wl,-rpath,./"
    )

    # X11
    if(TRAA_OPTION_ENABLE_X11)
        target_include_directories(${TRAA_LIBRARY} PRIVATE ${TRAA_X11_INCLUDE_DIRS})
        target_link_libraries(${TRAA_LIBRARY} PRIVATE ${TRAA_X11_LIBS})
    endif()
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
if(NOT IOS AND NOT VISIONOS AND NOT ANDROID)
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