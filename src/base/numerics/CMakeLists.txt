cmake_minimum_required(VERSION 3.10)

# set numerics files
set(TRAA_LIBRARY_BASE_NUMERICS_FILES
    "safe_compare.h"
    "moving_average.h"
    "moving_average.cc"
)

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_NUMERICS_FILES})

# set numerics library name
set(TRAA_LIBRARY_BASE_NUMERICS numerics)

# add numerics library
add_library(${TRAA_LIBRARY_BASE_NUMERICS} OBJECT)

# alias numerics to traa:base:numerics
add_library(traa::base::numerics ALIAS ${TRAA_LIBRARY_BASE_NUMERICS})

# add numerics files
target_sources(${TRAA_LIBRARY_BASE_NUMERICS} PRIVATE ${TRAA_LIBRARY_BASE_NUMERICS_FILES})

# set numerics folder
set_target_properties(${TRAA_LIBRARY_BASE_NUMERICS} PROPERTIES FOLDER base)
