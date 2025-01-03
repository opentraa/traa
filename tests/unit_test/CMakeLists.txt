cmake_minimum_required(VERSION 3.10)

include(${CMAKE_HOME_DIRECTORY}/cmake/utils.cmake)

enable_testing()

set(TRAA_UNIT_TEST unittest)

set(TRAA_UNIT_TEST_OUTPUT_NAME ${TRAA_UNIT_TEST})

file(GLOB_RECURSE TRAA_UNIT_TEST_FILES "${CMAKE_HOME_DIRECTORY}/src/*_unittest.*")

if(NOT WIN32)
    # exclude files in base/devices/screen/win
    list(FILTER TRAA_UNIT_TEST_FILES EXCLUDE REGEX "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/win")
endif()

# add base/devices/screen/test
file(GLOB TRAA_UNIT_TEST_SCREEN_TEST_FILES "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/*.*")

# add base/devices/screen/test/win
if(WIN32)
    file(GLOB TRAA_UNIT_TEST_SCREEN_TEST_FILES_WIN "${CMAKE_HOME_DIRECTORY}/src/base/devices/screen/test/win/*.*")
    list(APPEND TRAA_UNIT_TEST_SCREEN_TEST_FILES ${TRAA_UNIT_TEST_SCREEN_TEST_FILES_WIN})
endif()

list(APPEND TRAA_UNIT_TEST_FILES ${TRAA_UNIT_TEST_SCREEN_TEST_FILES})

source_group(TREE ${CMAKE_HOME_DIRECTORY}/src FILES ${TRAA_UNIT_TEST_FILES})

file(GLOB_RECURSE TRAA_UNIT_TEST_UTILS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/**)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${TRAA_UNIT_TEST_UTILS_FILES})

set(TRAA_UNIT_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TRAA_LIBRARY_INCLUDE_DIRS}
)

add_executable(${TRAA_UNIT_TEST}
    ${TRAA_UNIT_TEST_FILES}
    ${TRAA_UNIT_TEST_UTILS_FILES}
)

target_compile_definitions(${TRAA_UNIT_TEST} PRIVATE TRAA_UNIT_TEST)
target_link_libraries(${TRAA_UNIT_TEST} PRIVATE
    traa::base
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
    set(TRAA_UNIT_TEST_IDENTIFIER "io.traa.unittest")

    target_link_libraries(${TRAA_UNIT_TEST} PRIVATE "${TRAA_LIBRARY_FRAMEWORKS}")

    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER "${TRAA_UNIT_TEST_IDENTIFIER}"
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${TRAA_VERSION_SHORT}"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "${TRAA_VERSION_SHORT}"
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