cmake_minimum_required(VERSION 3.10)

# set strings files
set(TRAA_LIBRARY_BASE_STRINGS_FILES
    "ascii.cc"
    "ascii.h"
    "string_trans.h"
    "string_trans.cc"
)

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_STRINGS_FILES})

# set strings library name
set(TRAA_LIBRARY_BASE_STRINGS strings)

# add strings library
add_library(${TRAA_LIBRARY_BASE_STRINGS} OBJECT)

# alias strings to traa:base:strings
add_library(traa::base::strings ALIAS ${TRAA_LIBRARY_BASE_STRINGS})

# add strings files
target_sources(${TRAA_LIBRARY_BASE_STRINGS} PRIVATE ${TRAA_LIBRARY_BASE_STRINGS_FILES})

# set strings folder
set_target_properties(${TRAA_LIBRARY_BASE_STRINGS} PROPERTIES FOLDER base)
