set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TRAA_UNIT_TEST traa-unit-test)

set(TRAA_UNIT_TEST_OUTPUT_NAME ${TRAA_UNIT_TEST})

file(GLOB_RECURSE TRAA_UNIT_TEST_FILES "${CMAKE_HOME_DIRECTORY}/src/*_test.*")

# source_group(TREE ${CMAKE_HOME_DIRECTORY} PREFIX "test" FILES ${TRAA_UNIT_TEST_FILES})
source_group("test" FILES ${TRAA_UNIT_TEST_FILES})
source_group(TREE ${CMAKE_HOME_DIRECTORY} PREFIX "traa" FILES ${TRAA_LIBRARY_FILES})

add_executable(${TRAA_UNIT_TEST}
    ${TRAA_LIBRARY_FILES}
    ${TRAA_UNIT_TEST_FILES}
)
target_compile_definitions(${TRAA_UNIT_TEST} PRIVATE TRAA_UNIT_TEST)
target_compile_definitions(${TRAA_UNIT_TEST} PRIVATE SPDLOG_NO_EXCEPTIONS)
target_link_libraries(${TRAA_UNIT_TEST} PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
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
    absl::utility
)

target_include_directories(${TRAA_UNIT_TEST} PRIVATE "$<BUILD_INTERFACE:${TRAA_LIBRARY_INCLUDE_DIRS}>")
set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
    PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR}"
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
        CXX_VISIBILITY_PRESET hidden
        LINK_FLAGS "-Wl ${TRAA_UNIT_TEST_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
elseif(WIN32)
elseif(ANDROID)
    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
    )
    find_library(log-lib log)
    target_link_libraries(${TRAA_UNIT_TEST} PRIVATE ${log-lib})
elseif(UNIX)
    set_target_properties(${TRAA_UNIT_TEST} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LIBRARY_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        RUNTIME_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        PDB_OUTPUT_DIRECTORY "${TRAA_OUTPUT_DIR_REAL}"
        LINK_FLAGS "-Wl,-rpath,./"
    )
endif()