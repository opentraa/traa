cmake_minimum_required(VERSION 3.10)

enable_testing()

set(TRAA_SMOKE_TEST smoketest)
set(TRAA_SMOKE_TEST_OUTPUT_NAME ${TRAA_SMOKE_TEST})

file(GLOB_RECURSE TRAA_SMOKE_TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${TRAA_SMOKE_TEST_FILES})

set(TRAA_SMOKE_SIMPLE_WINDOW_FILES
    "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window.h"
)
if(APPLE)
    list(APPEND TRAA_SMOKE_SIMPLE_WINDOW_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_mac.mm"
    )
elseif(LINUX AND NOT ANDROID)
    list(APPEND TRAA_SMOKE_SIMPLE_WINDOW_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_linux.cc"
    )
elseif(WIN32)
    list(APPEND TRAA_SMOKE_SIMPLE_WINDOW_FILES
        "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/simple_window_win.cc"
    )
endif()
source_group(TREE ${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test FILES ${TRAA_SMOKE_SIMPLE_WINDOW_FILES})


set(TRAA_SMOKE_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test
    ${TRAA_LIBRARY_PUBLIC_HEADER_INCLUDE_DIR}
)

add_executable(${TRAA_SMOKE_TEST} ${TRAA_SMOKE_TEST_FILES} ${TRAA_SMOKE_SIMPLE_WINDOW_FILES})
target_link_libraries(${TRAA_SMOKE_TEST} PRIVATE
    traa::main
    gtest
    gtest_main
    gmock
    gmock_main)

target_include_directories(${TRAA_SMOKE_TEST} PRIVATE "$<BUILD_INTERFACE:${TRAA_SMOKE_TEST_INCLUDE_DIRS}>")
set_target_properties(${TRAA_SMOKE_TEST} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    PDB_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    OUTPUT_NAME "${TRAA_SMOKE_TEST_OUTPUT_NAME}"
)

# platform flags
if(APPLE)
    set(TRAA_SMOKE_TEST_IDENTIFIER "io.traa.smoketest")

    target_link_libraries(${TRAA_SMOKE_TEST} PRIVATE "${TRAA_LIBRARY_FRAMEWORKS}")

    set_target_properties(${TRAA_SMOKE_TEST} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/simple_window/mac/info.plist"
        MACOSX_FRAMEWORK_IDENTIFIER "${TRAA_SMOKE_TEST_IDENTIFIER}"
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${TRAA_VERSION_SHORT}"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "${TRAA_VERSION_SHORT}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TRAA_SMOKE_TEST_IDENTIFIER}"
        CXX_VISIBILITY_PRESET hidden
        LINK_FLAGS "-Wl ${TRAA_SMOKE_TEST_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_SMOKE_TEST} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
elseif(WIN32)
elseif(ANDROID)
    set_target_properties(${TRAA_SMOKE_TEST} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
    )
elseif(UNIX)
    set_target_properties(${TRAA_SMOKE_TEST} PROPERTIES
        LINK_FLAGS "-Wl,-rpath,./"
    )
endif()

include(GoogleTest)
gtest_discover_tests(${TRAA_SMOKE_TEST})