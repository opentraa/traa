cmake_minimum_required(VERSION 3.10)

set(TRAA_LIBRARY_BASE_THREAD_FILES
    "callback.h"
    "ffuture.h"
    "ffuture.cc"
    "task_queue.h"
    "thread_util.h"
    "waitable_future.h"
)

if(WIN32)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "thread_util_win.cc")
elseif(ANDROID)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "thread_util_linux.cc")
elseif(APPLE)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "thread_util_darwin.cc")
elseif(LINUX)
    list(APPEND TRAA_LIBRARY_BASE_THREAD_FILES "thread_util_linux.cc")
endif()

# set source group
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TRAA_LIBRARY_BASE_THREAD_FILES})

# set thread library name
set(TRAA_LIBRARY_BASE_THREAD thread)

# add thread library
add_library(${TRAA_LIBRARY_BASE_THREAD} OBJECT)

# alias thread to traa:base:thread
add_library(traa::base::thread ALIAS ${TRAA_LIBRARY_BASE_THREAD})

# add thread files
target_sources(${TRAA_LIBRARY_BASE_THREAD} PRIVATE ${TRAA_LIBRARY_BASE_THREAD_FILES})

# set thread folder
set_target_properties(${TRAA_LIBRARY_BASE_THREAD} PROPERTIES FOLDER base)
