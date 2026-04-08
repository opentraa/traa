cmake_minimum_required(VERSION 3.10)
project(traa_sdl_demo)

# Import SDL3
add_subdirectory(${CMAKE_HOME_DIRECTORY}/thirdparty/sdl ${CMAKE_CURRENT_BINARY_DIR}/sdl)

# Collect source files (exclude tests)
file(GLOB_RECURSE DEMO_SOURCES "*.cc" "*.h")
list(FILTER DEMO_SOURCES EXCLUDE REGEX ".*/tests/.*")

add_executable(traa_sdl_demo ${DEMO_SOURCES})
target_link_libraries(traa_sdl_demo PRIVATE traa::main SDL3::SDL3)
target_include_directories(traa_sdl_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_HOME_DIRECTORY}/include
)

set_target_properties(traa_sdl_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    FOLDER "examples"
)

# Tests
if(TRAA_OPTION_ENABLE_UNIT_TEST)
    add_subdirectory(tests)
endif()
