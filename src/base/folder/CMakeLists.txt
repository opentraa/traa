cmake_minimum_required(VERSION 3.10)

set(TRAA_LIBRARY_BASE_FOLDER_FILES
    "folder.h"
    "folder.cc"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "folder_win.cc")
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "folder_android.cc")
elseif(APPLE)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "folder_darwin.mm")
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_FOLDER_FILES "folder_linux.cc")
endif()

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_FOLDER_FILES})

# set folder library name
set(TRAA_LIBRARY_BASE_FOLDER folder)

# add folder library
add_library(${TRAA_LIBRARY_BASE_FOLDER} OBJECT)

# alias folder to traa:base:folder
add_library(traa::base::folder ALIAS ${TRAA_LIBRARY_BASE_FOLDER})

# add folder files
target_sources(${TRAA_LIBRARY_BASE_FOLDER} PRIVATE ${TRAA_LIBRARY_BASE_FOLDER_FILES})

# set folder folder
set_target_properties(${TRAA_LIBRARY_BASE_FOLDER} PROPERTIES FOLDER base)
