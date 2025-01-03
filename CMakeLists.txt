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
option(TRAA_OPTION_ENABLE_UNIT_TEST "Enable unit test" OFF)
option(TRAA_OPTION_ENABLE_SMOKE_TEST "Enable smoke test" OFF)

if(LINUX)
    # To enable x11 you have install x11 and x11-xext and x11-xcomposite development packages.
    # sudo apt-get install libx11-dev libxext-dev libxcomposite-dev
    option(TRAA_OPTION_ENABLE_X11 "Enable X11 support" ON)
    
    if(TRAA_OPTION_ENABLE_X11)
        find_package(X11)
        if(X11_FOUND AND X11_Xext_FOUND AND X11_Xcomposite_FOUND AND X11_Xrandr_FOUND)
            set(TRAA_OPTION_ENABLE_X11 ON)
            add_definitions(-DTRAA_ENABLE_X11)
        else()
            message(WARNING "[TRAA] X11 not found, disable X11 support")
            set(TRAA_OPTION_ENABLE_X11 OFF)
        endif()
    endif()

    # TODO(@sylar) COPY FINDERS FROM OBS PROJECT AND USE list(APPEND CMAKE_MODULE_PATH "${CMAKE_HOME_DIRECTORY}/cmake/finders") TO INCLUDE THEM
    option(TRAA_OPTION_ENABLE_WAYLAND "Enable Wayland support" OFF)
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
        /wd4702\
        /wd4995")
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

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=stringop-truncation")
    else()
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
    # do not use this anymore, we will use dsymutil and strip to generate dsym file and strip debug and local symbols
    # set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
elseif(IOS)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Oz")
elseif(ANDROID)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Oz")
endif()

message(STATUS "[TRAA] ${CMAKE_CXX_FLAGS}")

# Define output directory.
if(NOT DEFINED TRAA_OPTION_BIN_FOLDER OR TRAA_OPTION_BIN_FOLDER STREQUAL "")
    set(TRAA_OPTION_BIN_FOLDER "${CMAKE_HOME_DIRECTORY}/bin")
endif()

if(WIN32)
    # make lower case of CMAKE_GENERATOR_PLATFORM for windows, and replace win32 with x86
    string(TOLOWER ${CMAKE_GENERATOR_PLATFORM} LOWERCASE_CMAKE_GENERATOR_PLATFORM)
    if(${LOWERCASE_CMAKE_GENERATOR_PLATFORM} STREQUAL "win32")
        set(TRAA_OPTION_BIN_FOLDER "${TRAA_OPTION_BIN_FOLDER}/x86")
    endif()
    set(TRAA_ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OPTION_BIN_FOLDER}/lib/${LOWERCASE_CMAKE_GENERATOR_PLATFORM}$<0:>")
elseif(APPLE)
    set(TRAA_ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OPTION_BIN_FOLDER}$<0:>")
elseif(ANDROID)
    set(TRAA_ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OPTION_BIN_FOLDER}/lib/${ANDROID_ABI}$<0:>")
elseif(LINUX)
    set(TRAA_ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OPTION_BIN_FOLDER}/lib$<0:>")
endif()

message(STATUS "[TRAA] TRAA_ARCHIVE_OUTPUT_DIRECTORY: ${TRAA_ARCHIVE_OUTPUT_DIRECTORY}")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(TRAA_DEBUG)
endif()

# ##############################################################################
# TRAA Library Section
# ##############################################################################

# public header files
set(TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR ${CMAKE_HOME_DIRECTORY}/include)
file(GLOB_RECURSE TRAA_LIBRARY_PUBLIC_HEADER_FILES ${TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR}/*.h)

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
source_group(TREE ${TRAA_THIRDPARTY_JSON_INCLUDE_DIR}/nlohmann FILES ${TRAA_THIRDPARTY_JSON_FILE})
add_library(nlohmann_json OBJECT ${TRAA_THIRDPARTY_JSON_FILE})
add_library(traa::base::json ALIAS nlohmann_json)
set_target_properties(nlohmann_json PROPERTIES FOLDER thirdparty)
set_target_properties(nlohmann_json PROPERTIES LINKER_LANGUAGE CXX)

# thirdparty libyuv-static
add_definitions(-DLIBYUV_DISABLE_NEON=1) # TODO @sylar: how to enable neon?
add_definitions(-DLIBYUV_DISABLE_SVE=1) # TODO @sylar: how to enable sve?
add_definitions(-DLIBYUV_DISABLE_SME=1) # TODO @sylar: how to enable sme?
add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/libyuv-static)
set(TRAA_THIRDPARTY_LIBYUV_INCLUDE_DIR "${LIBYUV_INCLUDE_DIR}")
set_property(TARGET yuv PROPERTY FOLDER "thirdparty")

# TODO @sylar: do we really need absl?
# thirdparty absil-cpp
if(TRAA_ENABLE_ABSL)
    add_subdirectory(thirdparty/abseil-cpp)
    set(TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/abseil-cpp")
endif()

# thirdparty asio
add_compile_definitions(ASIO_NO_EXCEPTIONS)
add_compile_definitions(ASIO_NO_TYPEID)
add_compile_definitions(ASIO_HEADER_ONLY)
set(TRAA_THIRDPARTY_ASIO_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/asio/asio/include")
file(GLOB_RECURSE TRAA_THIRDPARTY_ASIO_FILES
    ${CMAKE_HOME_DIRECTORY}/thirdparty/asio/asio/include/asio/*.hpp
)
source_group(TREE ${TRAA_THIRDPARTY_ASIO_INCLUDE_DIR}/asio FILES ${TRAA_THIRDPARTY_ASIO_FILES})
add_library(asio OBJECT ${TRAA_THIRDPARTY_ASIO_FILES})
add_library(traa::base::asio ALIAS asio)
set_target_properties(asio PROPERTIES FOLDER thirdparty)
set_target_properties(asio PROPERTIES LINKER_LANGUAGE CXX)

# thirdparty cpu_features
if(ANDROID)
    # The cpu_features library re-implements getauxval() for Android API < 18 by parsing /proc/self/auxv
    # directly, since getauxval() was only added in API 18. However, their implementation avoids including
    # <sys/auxv.h>, which means the AT_* constants are undefined.
    #
    # These constants are normally defined in $ANDROID_NDK_HOME/sysroot/usr/include/linux/auxvec.h.
    # As a workaround until we can properly fix this upstream in cpu_features, we define them here
    # when targeting older Android API levels.
    #
    # Values from linux/auxvec.h:
    # AT_HWCAP = 16        // CPU capability bit flags
    # AT_PLATFORM = 15     // String identifying platform
    # AT_HWCAP2 = 26       // Extension of AT_HWCAP
    # AT_BASE_PLATFORM = 24 // String identifying real platform
    #
    # Define AT_* constants for Android API < 18 since getauxval() and its constants were only added in API 18.
    # These constants are needed by cpu_features library to detect CPU capabilities on older Android versions.
    message(STATUS "ANDROID_PLATFORM_LEVEL: ${ANDROID_PLATFORM_LEVEL}")
    if(${ANDROID_PLATFORM_LEVEL} LESS 18)
        add_definitions(-DAT_HWCAP=16)
        add_definitions(-DAT_HWCAP2=26)
        add_definitions(-DAT_PLATFORM=15)
        add_definitions(-DAT_BASE_PLATFORM=24)
    endif()
endif(ANDROID)
add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/cpu_features)
set(TRAA_THIRDPARTY_CPU_FEATURES_INCLUDE_DIR "${CMAKE_HOME_DIRECTORY}/thirdparty/cpu_features")
set(TRAA_THIRDPARTY_CPU_FEATURES_INCLUDE_DIR_NDK_COMPAT "${CMAKE_HOME_DIRECTORY}/thirdparty/cpu_features/ndk_compat")
set_property(TARGET cpu_features PROPERTY FOLDER "thirdparty")
set_property(TARGET cpu_features_utils PROPERTY FOLDER "thirdparty")
if(UNIX)
    set_property(TARGET cpu_features_hardware_detection PROPERTY FOLDER "thirdparty")
endif()


# library include directories
set(TRAA_LIBRARY_INCLUDE_DIRS
    ${TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_SPDLOG_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_JSON_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_LIBYUV_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_ASIO_INCLUDE_DIR}
    ${TRAA_THIRDPARTY_CPU_FEATURES_INCLUDE_DIR} 
    ${TRAA_THIRDPARTY_CPU_FEATURES_INCLUDE_DIR_NDK_COMPAT}
    ${TRAA_THIRDPARTY_ABSEIL_CPP_INCLUDE_DIR}
    "${CMAKE_HOME_DIRECTORY}/src"
)

include_directories(${TRAA_LIBRARY_INCLUDE_DIRS})

# platform frameworks, TODO @sylar: where should we put this?
# this will be used by traa::main and traa::unittest
if(APPLE)
    set(TRAA_LIBRARY_FRAMEWORKS "-framework Foundation")
    if(IOS)
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework UIKit")
    elseif(MACOS)
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework AppKit")
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework CoreVideo")
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework IOKit")
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework IOSurface")
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-framework CoreMedia")
        list(APPEND TRAA_LIBRARY_FRAMEWORKS "-weak_framework ScreenCaptureKit") # macOS 12.3
    endif()
endif()


# add traa::main and traa::base
add_subdirectory(${CMAKE_HOME_DIRECTORY}/src)

# add test modules
if(TRAA_OPTION_ENABLE_UNIT_TEST OR TRAA_OPTION_ENABLE_SMOKE_TEST)
    if(NOT IOS AND NOT VISIONOS AND NOT ANDROID)
        add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/googletest)
        set_property(TARGET gtest PROPERTY FOLDER "thirdparty/googletest")
        set_property(TARGET gtest_main PROPERTY FOLDER "thirdparty/googletest")
        set_property(TARGET gmock PROPERTY FOLDER "thirdparty/googletest")
        set_property(TARGET gmock_main PROPERTY FOLDER "thirdparty/googletest")

        if(TRAA_OPTION_ENABLE_UNIT_TEST)
            add_subdirectory(${CMAKE_HOME_DIRECTORY}/tests/unit_test)
        endif()

        if(TRAA_OPTION_ENABLE_SMOKE_TEST)
            add_subdirectory(${CMAKE_HOME_DIRECTORY}/tests/smoke_test)
        endif()
    endif()
endif()