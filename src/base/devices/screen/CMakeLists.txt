cmake_minimum_required(VERSION 3.10)

if(WIN32 OR MACOS OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES
        "blank_detector_desktop_capturer_wrapper.cc"
        "blank_detector_desktop_capturer_wrapper.h"
        "cropped_desktop_frame.cc"
        "cropped_desktop_frame.h"
        "delegated_source_list_controller.h"
        "desktop_capture_options.cc"
        "desktop_capture_options.h"
        "desktop_capture_types.h"
        "desktop_capturer_differ_wrapper.cc"
        "desktop_capturer_differ_wrapper.h"
        "desktop_capturer.cc"
        "desktop_capturer.h"
        "desktop_frame_rotation.h"
        "desktop_frame_rotation.cc"
        "desktop_frame.h"
        "desktop_frame.cc"
        "desktop_geometry.h"
        "desktop_geometry.cc"
        "desktop_region.h"
        "desktop_region.cc"
        "differ_block.cc"
        "differ_block.h"
        "enumerator.h"
        "enumerator.cc"
        "fallback_desktop_capturer_wrapper.cc"
        "fallback_desktop_capturer_wrapper.h"
        "full_screen_application_handler.cc"
        "full_screen_application_handler.h"
        "full_screen_window_detector.cc"
        "full_screen_window_detector.h"
        "mouse_cursor.h"
        "mouse_cursor.cc"
        "resolution_tracker.h"
        "resolution_tracker.cc"
        "rgba_color.h"
        "rgba_color.cc"
        "screen_capture_frame_queue.h"
        "screen_capturer_helper.h"
        "screen_capturer_helper.cc"
        "shared_desktop_frame.cc"
        "shared_desktop_frame.h"
        "shared_memory.h"
        "shared_memory.cc"
        "utils.h"
        "utils.cc"
        "window_finder.h"
    )

    set(TRAA_ENABLE_SSE2 OFF)
    message(STATUS "[TRAA] CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "[TRAA] CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
    if(WIN32 AND CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
        set(TRAA_ENABLE_SSE2 ON)
    elseif(APPLE)
        message(STATUS "[TRAA] ARCHS: ${ARCHS}")
        # for APPLE, from cmake/ios.toolchain.cmake, ARCHS is set to:
        # 'x86_64 arm64' (MAC_UNIVERSAL)
        # 'x86_64' (MAC)
        # 'arm64' (MAC_ARM64)
        # we do not support i386 any more
        # so we just check if the ${ARCHS} contains 'x86_64
        if("x86_64" IN_LIST ARCHS)
            set(TRAA_ENABLE_SSE2 ON)
            # When ARCHS contains multiple architectures, we cannot enable SSE2 separately for each arch during cmake build
            # So we enable SSE2 whenever x86_64 is detected, even though we may also be building for arm64
            # We'll need to use #if defined(TRAA_ARCH_X86_FAMILY) in the source files to conditionally enable SSE2
        endif()
    elseif(UNIX)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(i386)|(^i686)")
            include(CheckCXXSourceCompiles)
            set(SSE2_TEST_SOURCE "
                #include <emmintrin.h>
                int main() {
                    __m128i zero = _mm_setzero_si128();
                    return 0;
                }
            ")
            
            set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -msse2")
            check_cxx_source_compiles("${SSE2_TEST_SOURCE}" SSE2_COMPILE_TEST)
            
            if(SSE2_COMPILE_TEST)
                set(TRAA_ENABLE_SSE2 ON)
            endif()
        endif()
    endif()
    
    if(TRAA_ENABLE_SSE2)
        message(STATUS "[TRAA] SSE2 support enabled")
        add_definitions(-DTRAA_ENABLE_SSE2)
        
        if(NOT MSVC)
            #TODO @sylar: we may should make a new cmake target to compile sse2 code
            # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
        endif()
        
        list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES "differ_vector_sse2.cc")
        list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES "differ_vector_sse2.h")
    else()
        message(STATUS "[TRAA] SSE2 support disabled")
    endif()
endif()

if(WIN32)
    # dxgi
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
        "win/dxgi/dxgi_adapter_duplicator.h"
        "win/dxgi/dxgi_adapter_duplicator.cc"
        "win/dxgi/dxgi_context.h"
        "win/dxgi/dxgi_context.cc"
        "win/dxgi/dxgi_duplicator_controller.h"
        "win/dxgi/dxgi_duplicator_controller.cc"
        "win/dxgi/dxgi_frame.h"
        "win/dxgi/dxgi_frame.cc"
        "win/dxgi/dxgi_output_duplicator.h"
        "win/dxgi/dxgi_output_duplicator.cc"
        "win/dxgi/dxgi_texture_mapping.h"
        "win/dxgi/dxgi_texture_mapping.cc"
        "win/dxgi/dxgi_texture_staging.h"
        "win/dxgi/dxgi_texture_staging.cc"
        "win/dxgi/dxgi_texture.h"
        "win/dxgi/dxgi_texture.cc"
    )

    # wgc
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
        "win/wgc/wgc_capture_session.h"
        "win/wgc/wgc_capture_session.cc"
        "win/wgc/wgc_capture_source.h"
        "win/wgc/wgc_capture_source.cc"
        "win/wgc/wgc_capturer_win.h"
        "win/wgc/wgc_capturer_win.cc"
        "win/wgc/wgc_desktop_frame.h"
        "win/wgc/wgc_desktop_frame.cc"
    )

    # commons
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
        "win/capture_utils.h"
        "win/capture_utils.cc"
        "win/cropping_window_capturer_win.cc"
        "win/cropping_window_capturer.cc"
        "win/cropping_window_capturer.h"
        "win/cursor.h"
        "win/cursor.cc"
        "win/d3d_device.h"
        "win/d3d_device.cc"
        "win/desktop_capture_utils.h"
        "win/desktop_capture_utils.cc"
        "win/desktop_frame_win.h"
        "win/desktop_frame_win.cc"
        "win/display_configuration_monitor.h"
        "win/display_configuration_monitor.cc"
        "win/enumerator_win.cc"
        "win/full_screen_win_application_handler.cc"
        "win/full_screen_win_application_handler.h"
        "win/scoped_gdi_object.h"
        "win/scoped_thread_desktop.cc"
        "win/scoped_thread_desktop.h"
        "win/screen_capturer_win_directx.cc"
        "win/screen_capturer_win_directx.h"
        "win/screen_capturer_win_gdi.cc"
        "win/screen_capturer_win_gdi.h"
        "win/screen_capturer_win.cc"
        "win/selected_window_context.cc"
        "win/selected_window_context.h"
        "win/thread_desktop.cc"
        "win/thread_desktop.h"
        "win/window_capturer_win_gdi.cc"
        "win/window_capturer_win_gdi.h"
        "win/window_capturer_win.cc"
        "win/window_finder_win.h"
        "win/window_finder_win.cc"
    )
elseif(MACOS)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
        "darwin/desktop_configuration_monitor.cc"
        "darwin/desktop_configuration_monitor.h"
        "darwin/desktop_configuration.h"
        "darwin/desktop_configuration.mm"
        "darwin/desktop_frame_cgimage.h"
        "darwin/desktop_frame_cgimage.mm"
        "darwin/desktop_frame_iosurface.h"
        "darwin/desktop_frame_iosurface.mm"
        "darwin/desktop_frame_provider.h"
        "darwin/desktop_frame_provider.mm"
        "darwin/desktop_frame_utils.h"
        "darwin/desktop_frame_utils.cc"
        "darwin/enumerator_darwin.mm"
        "darwin/full_screen_mac_application_handler.cc"
        "darwin/full_screen_mac_application_handler.h"
        "darwin/screen_capturer_darwin.mm"
        "darwin/screen_capturer_mac.h"
        "darwin/screen_capturer_mac.mm"
        "darwin/screen_capturer_sck.h"
        "darwin/screen_capturer_sck.mm"
        "darwin/window_capturer_mac.mm"
        "darwin/window_finder_mac.h"
        "darwin/window_finder_mac.mm"
        "darwin/window_list_utils.cc"
        "darwin/window_list_utils.h"
    )
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
        "linux/capture_utils.h"
        "linux/capture_utils.cc"
        "linux/enumerator_linux.cc"
    )

    # Add X11 support if TRAA_OPTION_ENABLE_X11 is set
    if(TRAA_OPTION_ENABLE_X11)
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

        list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
            "linux/x11/shared_x_display.cc"
            "linux/x11/shared_x_display.h"
            "linux/x11/x_atom_cache.h"
            "linux/x11/x_atom_cache.cc"
            "linux/x11/x_error_trap.h"
            "linux/x11/x_error_trap.cc"
            "linux/x11/x_server_pixel_buffer.h"
            "linux/x11/x_server_pixel_buffer.cc"
            "linux/x11/x_window_list_utils.h"
            "linux/x11/x_window_list_utils.cc"
            "linux/x11/x_window_property.h"
            "linux/x11/x_window_property.cc"
        )
    endif()

    # Enable wayland if TRAA_OPTION_ENABLE_WAYLAND is set
    if(TRAA_OPTION_ENABLE_WAYLAND)
        add_definitions(-DTRAA_ENABLE_WAYLAND)
    endif()

    if(NOT TRAA_OPTION_ENABLE_WAYLAND AND NOT TRAA_OPTION_ENABLE_X11)
        list(APPEND TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES 
            "screen_capturer_null.cc"
            "window_capturer_null.cc"
        )
    endif()
endif()

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES})

# set devices library name
set(TRAA_LIBRARY_BASE_DEVICES_SCREEN screen)

# add devices library
add_library(${TRAA_LIBRARY_BASE_DEVICES_SCREEN} OBJECT)

# alias devices to traa:base:devices::screen
add_library(traa::base::devices::screen ALIAS ${TRAA_LIBRARY_BASE_DEVICES_SCREEN})

# add devices files
target_sources(${TRAA_LIBRARY_BASE_DEVICES_SCREEN} PRIVATE ${TRAA_LIBRARY_BASE_DEVICES_SCREEN_FILES})

# set devices folder
set_target_properties(${TRAA_LIBRARY_BASE_DEVICES_SCREEN} PROPERTIES FOLDER base/devices)
