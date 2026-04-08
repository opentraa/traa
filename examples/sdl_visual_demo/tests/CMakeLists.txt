cmake_minimum_required(VERSION 3.10)

enable_testing()

# Collect test source files
file(GLOB_RECURSE SDL_DEMO_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

# Collect source files to test (exclude main.cc and tests/)
get_filename_component(SDL_DEMO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE SDL_DEMO_SRC_FILES "${SDL_DEMO_SRC_DIR}/*.cc")
list(FILTER SDL_DEMO_SRC_FILES EXCLUDE REGEX ".*/tests/.*")
list(FILTER SDL_DEMO_SRC_FILES EXCLUDE REGEX ".*/main\\.cc$")

add_executable(traa_sdl_demo_test ${SDL_DEMO_TEST_FILES} ${SDL_DEMO_SRC_FILES})

target_include_directories(traa_sdl_demo_test PRIVATE
    ${SDL_DEMO_SRC_DIR}
    ${CMAKE_HOME_DIRECTORY}/include
)

target_link_libraries(traa_sdl_demo_test PRIVATE
    traa::main
    SDL3::SDL3
    gtest
    gtest_main
)

set_target_properties(traa_sdl_demo_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    FOLDER "examples/tests"
)

include(GoogleTest)
gtest_discover_tests(traa_sdl_demo_test)
