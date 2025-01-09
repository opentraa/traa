cmake_minimum_required(VERSION 3.10)

# set base64 files
set(TRAA_LIBRARY_BASE_BASE64_FILES
    "base64.h"
    "base64.cc"
)

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_BASE64_FILES})

# set base64 library name
set(TRAA_LIBRARY_BASE_BASE64 base64)

# add base64 library
add_library(${TRAA_LIBRARY_BASE_BASE64} OBJECT)

# alias base64 to traa:base:base64
add_library(traa::base::base64 ALIAS ${TRAA_LIBRARY_BASE_BASE64})

# add base64 files
target_sources(${TRAA_LIBRARY_BASE_BASE64} PRIVATE ${TRAA_LIBRARY_BASE_BASE64_FILES})

# set base64 folder
set_target_properties(${TRAA_LIBRARY_BASE_BASE64} PROPERTIES FOLDER base)
