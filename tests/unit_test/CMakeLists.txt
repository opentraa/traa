cmake_minimum_required(VERSION 3.10)

include(${CMAKE_HOME_DIRECTORY}/cmake/utils.cmake)

enable_testing()

set(TRAA_UNIT_TEST unittest)

set(TRAA_UNIT_TEST_OUTPUT_NAME ${TRAA_UNIT_TEST})

set(TRAA_UNIT_TEST_FILES "")

# add traa::base::screen
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/desktop_frame_generator.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/desktop_frame_generator.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/desktop_frame_generator.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/fake_desktop_capturer.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/fake_desktop_capturer.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/mock_desktop_capturer_callback.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/mock_desktop_capturer_callback.h"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/test_utils_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/test_utils.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/test_utils.h"
    
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/blank_detector_desktop_capturer_wrapper_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/cropped_desktop_frame_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_capturer_differ_wrapper_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_frame_rotation_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_frame_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_geometry_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/desktop_region_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/differ_block_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/fallback_desktop_capturer_wrapper_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/rgba_color_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/screen_capturer_helper_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/screen_capturer_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/window_capturer_unittest.cc"
)

if(APPLE)
    list(APPEND TRAA_UNIT_TEST_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_mac.mm"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_mac.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_unittest.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_lock_posix.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_lock_posix.h"

        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/darwin/screen_capturer_mac_unittest.cc"

        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/screen_capturer_integration_test.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/window_finder_unittest.cc"
    )
elseif(LINUX AND NOT ANDROID)
    list(APPEND TRAA_UNIT_TEST_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_linux.cc"
    )
    if(TRAA_OPTION_ENABLE_X11)
        list(APPEND TRAA_UNIT_TEST_FILES
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_linux.cc"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_unittest.cc"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.cc"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.h"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_lock_posix.cc"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_lock_posix.h"

            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/screen_capturer_integration_test.cc"
            "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/window_finder_unittest.cc"
        )
    endif()
elseif(WIN32)
    list(APPEND TRAA_UNIT_TEST_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_win.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_win.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/win/test_window.h"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/win/test_window.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer_unittest.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/screen_drawer.h"

        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/wgc/wgc_capture_source_unittest.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/wgc/wgc_capturer_win_unittest.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/cursor_unittest.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win/screen_capturer_win_directx_unittest.cc"

        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/screen_capturer_integration_test.cc"
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/window_finder_unittest.cc"
    )
endif()

# add traa::base::numerics
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/numerics/moving_average_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/numerics/safe_compare_unittest.cc"
)

# add traa::base::system
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/system/metrics_default_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/system/metrics_unittest.cc"
)

# add traa::base::thread
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/callback_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/ffuture_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/rw_lock_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/task_queue_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/thread_util_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/thread/waitable_future_unittest.cc"
)

# add traa::base::utils
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/utils/time_utils_unittest.cc"
)

# add traa::base
list(APPEND TRAA_UNIT_TEST_FILES 
    "${CMAKE_HOME_DIRECTORY}/src/base/checks_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/platform_thread_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/random_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/string_to_number_unittest.cc"
    "${CMAKE_HOME_DIRECTORY}/src/base/string_utils_unittest.cc"
)
source_group(TREE ${CMAKE_HOME_DIRECTORY}/src FILES ${TRAA_UNIT_TEST_FILES})

file(GLOB_RECURSE TRAA_UNIT_TEST_MAIN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_UNIT_TEST_MAIN_FILES})

set(TRAA_UNIT_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TRAA_LIBRARY_INCLUDE_DIRS}
)

add_executable(${TRAA_UNIT_TEST}
    ${TRAA_UNIT_TEST_FILES}
    ${TRAA_UNIT_TEST_MAIN_FILES}
)
add_executable(traa::unittest ALIAS ${TRAA_UNIT_TEST})

target_compile_definitions(${TRAA_UNIT_TEST} PRIVATE TRAA_UNIT_TEST)

target_link_libraries(${TRAA_UNIT_TEST} PRIVATE
    traa::base::core
    yuv
    gtest
    gtest_main
    gmock
    gmock_main)

target_include_directories(${TRAA_UNIT_TEST} PRIVATE "$<BUILD_INTERFACE:${TRAA_UNIT_TEST_INCLUDE_DIRS}>")
set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    PDB_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    OUTPUT_NAME "${TRAA_UNIT_TEST_OUTPUT_NAME}"
)

# platform flags
if(APPLE)
    set(TRAA_UNIT_TEST_IDENTIFIER "org.opentraa.traa.unittest")

    target_link_libraries(${TRAA_UNIT_TEST} PRIVATE "${TRAA_LIBRARY_FRAMEWORKS}")

    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/mac/info.plist"
        MACOSX_FRAMEWORK_IDENTIFIER "${TRAA_UNIT_TEST_IDENTIFIER}"
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${TRAA_OPTION_VERSION}"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "${TRAA_OPTION_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TRAA_UNIT_TEST_IDENTIFIER}"
        CXX_VISIBILITY_PRESET hidden
        LINK_FLAGS "-Wl ${TRAA_UNIT_TEST_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
elseif(WIN32)
    target_link_libraries(${TRAA_UNIT_TEST} PRIVATE
        dwmapi.lib
        Shcore.lib
        dxgi.lib
        d3d11.lib
        Winmm.lib
    )
elseif(ANDROID)
    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
    )
    find_library(log-lib log)
    target_link_libraries(${TRAA_UNIT_TEST} PRIVATE ${log-lib})
elseif(LINUX)
    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        LINK_FLAGS "-Wl,-rpath,./"
    )
     # X11
     if(TRAA_OPTION_ENABLE_X11)
        target_include_directories(${TRAA_UNIT_TEST} PRIVATE ${TRAA_X11_INCLUDE_DIRS})
        target_link_libraries(${TRAA_UNIT_TEST} PRIVATE ${TRAA_X11_LIBS})
    endif()
endif()

# Enable ctest
include(GoogleTest)
gtest_discover_tests(${TRAA_UNIT_TEST})

# Enable sanitizer for only Linux and MacOS
if(MACOS OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    traa_enable_sanitizer(${TRAA_UNIT_TEST})

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170
        # on the official document, , /MTd is supported but the version of msvc on our CI is too old
        # so that the /MTd and -fsanitize=address can't be used together
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    endif()
endif()