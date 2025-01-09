cmake_minimum_required(VERSION 3.10)

project(main)
set(TRAA_LIBRARY_MAIN_OUTPUT_NAME "traa")

# set main files
set(TRAA_LIBRARY_MAIN_FILES
    "utils/obj_string.h"
    "engine.h"
    "engine.cc"
    "main.cc"
    "traa.cc"
)

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_MAIN_FILES})

# must be after source_group
if(WIN32)
    # resource files
    configure_file(${CMAKE_HOME_DIRECTORY}/resources/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)
    list(APPEND TRAA_LIBRARY_MAIN_FILES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

# set main library name
set(TRAA_LIBRARY_MAIN main)

# add main library
add_library(${TRAA_LIBRARY_MAIN} SHARED)

# alias main to traa:main
add_library(traa::main ALIAS ${TRAA_LIBRARY_MAIN})

# add main files
target_sources(${TRAA_LIBRARY_MAIN} PRIVATE ${TRAA_LIBRARY_MAIN_FILES})

# link traa::base
target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE traa::base::core)

# link yuv
target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE yuv)

# link cpu_features
target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE cpu_features)
if(ANDROID)
    target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE cpu_features_ndk_compat)
endif()

# thirdparty link
if(TRAA_ENABLE_ABSL)
    target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE
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

# add compile definitions
target_compile_definitions(${TRAA_LIBRARY_MAIN} PUBLIC TRAA_EXPORT)
set_target_properties(${TRAA_LIBRARY_MAIN} PROPERTIES
    VERSION "${TRAA_VERSION_SHORT}"
    SOVERSION "${TRAA_VERSION_SHORT}"
    PUBLIC_HEADER "${TRAA_LIBRARY_PUBLIC_HEADER_FILES}"
    ARCHIVE_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    PDB_OUTPUT_DIRECTORY "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}"
    OUTPUT_NAME "${TRAA_LIBRARY_MAIN_OUTPUT_NAME}"
)

# platform flags
if(APPLE)
    set(TRAA_LIBRARY_IDENTIFIER "io.traa.core")

    target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE "${TRAA_LIBRARY_FRAMEWORKS}")

    set_target_properties(${TRAA_LIBRARY_MAIN} PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER "${TRAA_LIBRARY_IDENTIFIER}"
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${TRAA_VERSION_SHORT}"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "${TRAA_VERSION_SHORT}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${TRAA_LIBRARY_IDENTIFIER}"
        LINK_FLAGS "-Wl ${TRAA_LIBRARY_FRAMEWORKS_FOLDERS} -rpath @loader_path"
    )

    set_target_properties(${TRAA_LIBRARY_MAIN} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")


    # dsymutil to generate dsym file
    add_custom_command(
        TARGET ${TRAA_LIBRARY_MAIN}
        POST_BUILD
        COMMAND dsymutil
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/${TRAA_LIBRARY_MAIN_OUTPUT_NAME}.framework/${TRAA_LIBRARY_MAIN_OUTPUT_NAME}" 
        "-o" 
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/${TRAA_LIBRARY_MAIN_OUTPUT_NAME}.framework.dSYM"
        COMMENT "Generate dsym file done on final binary."
    )

    # strip to strip debug and local symbols
    add_custom_command(
        TARGET ${TRAA_LIBRARY_MAIN}
        POST_BUILD
        COMMAND strip -r -S -x
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/${TRAA_LIBRARY_MAIN_OUTPUT_NAME}.framework/${TRAA_LIBRARY_MAIN_OUTPUT_NAME}"
        COMMENT "Strip debug and local symbols done on final binary."
    )
elseif(WIN32)
    target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE
        dwmapi.lib
        Shcore.lib
        dxgi.lib
        d3d11.lib
        Winmm.lib
    )
elseif(ANDROID)
    find_library(log-lib log)
    target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE ${log-lib})

    add_custom_command(
        TARGET ${TRAA_LIBRARY_MAIN}
        POST_BUILD
        COMMAND cp -RP
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/lib${TRAA_LIBRARY_MAIN_OUTPUT_NAME}.so"
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/lib${TRAA_LIBRARY_MAIN_OUTPUT_NAME}-symbol.so"
    )
    add_custom_command(
        TARGET ${TRAA_LIBRARY_MAIN}
        POST_BUILD
        COMMAND "${ANDROID_TOOLCHAIN_ROOT}/bin/llvm-strip" --strip-all
        "${TRAA_ARCHIVE_OUTPUT_DIRECTORY}/lib${TRAA_LIBRARY_MAIN_OUTPUT_NAME}.so"
        COMMENT "Strip debug symbols done on final binary."
    )
elseif(LINUX)
    set_target_properties(${TRAA_LIBRARY_MAIN} PROPERTIES
        LINK_FLAGS "-Wl,-rpath,./"
    )

    # X11
    if(TRAA_OPTION_ENABLE_X11)
        target_include_directories(${TRAA_LIBRARY_MAIN} PRIVATE ${TRAA_X11_INCLUDE_DIRS})
        target_link_libraries(${TRAA_LIBRARY_MAIN} PRIVATE ${TRAA_X11_LIBS})
    endif()
endif()

# actions after build
if(NOT APPLE)
    set(TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR "${TRAA_OPTION_BIN_FOLDER}/include")
    file(MAKE_DIRECTORY "${TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${TRAA_LIBRARY_PUBLIC_HEADER_FILES} "${TRAA_LIBRARY_PUBLIC_HEADER_TARGET_INCLUDE_DIR}"
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
    )
endif()