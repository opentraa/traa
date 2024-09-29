# CMakeLists for libyuv
# Since the newest version of libyuv does not support building as a static library with CMake on macOS well, we need to build it by ourselves.

cmake_minimum_required(VERSION 3.10)

include(CheckCSourceCompiles)

# Name project libyuv
project(libyuv C CXX)
message(STATUS "[TRAA] Building libyuv")

# Reuqire C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable NEON optimizations.
option(LIBYUV_ENABLE_NEON "Enable NEON optimizations" OFF)

# Enable SVE optimizations.
option(LIBYUV_ENABLE_SVE "Enable SVE optimizations" OFF)

# Enable SME optimizations.
option(LIBYUV_ENABLE_SME "Enable SME optimizations" OFF)

set(LIBYUV yuv)
set(LIBYUV_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/yuv)
set(LIBYUV_SOURCE_DIR ${LIBYUV_BASE_DIR}/source)
set(LIBYUV_INCLUDE_DIR ${LIBYUV_BASE_DIR}/include)
set(LIBYUV_INCLUDE_DIR ${LIBYUV_INCLUDE_DIR} PARENT_SCOPE)

set(LIBYUV_COMMON_SOURCE_FILES
    ${LIBYUV_SOURCE_DIR}/compare.cc
    ${LIBYUV_SOURCE_DIR}/compare_common.cc
    ${LIBYUV_SOURCE_DIR}/compare_gcc.cc
    ${LIBYUV_SOURCE_DIR}/compare_msa.cc
    ${LIBYUV_SOURCE_DIR}/compare_win.cc
    ${LIBYUV_SOURCE_DIR}/convert_argb.cc
    ${LIBYUV_SOURCE_DIR}/convert.cc
    ${LIBYUV_SOURCE_DIR}/convert_from_argb.cc
    ${LIBYUV_SOURCE_DIR}/convert_from.cc
    ${LIBYUV_SOURCE_DIR}/convert_jpeg.cc
    ${LIBYUV_SOURCE_DIR}/convert_to_argb.cc
    ${LIBYUV_SOURCE_DIR}/convert_to_i420.cc
    ${LIBYUV_SOURCE_DIR}/cpu_id.cc
    ${LIBYUV_SOURCE_DIR}/mjpeg_decoder.cc
    ${LIBYUV_SOURCE_DIR}/mjpeg_validate.cc
    ${LIBYUV_SOURCE_DIR}/planar_functions.cc
    ${LIBYUV_SOURCE_DIR}/rotate_any.cc
    ${LIBYUV_SOURCE_DIR}/rotate_argb.cc
    ${LIBYUV_SOURCE_DIR}/rotate.cc
    ${LIBYUV_SOURCE_DIR}/rotate_common.cc
    ${LIBYUV_SOURCE_DIR}/rotate_gcc.cc
    ${LIBYUV_SOURCE_DIR}/rotate_lsx.cc
    ${LIBYUV_SOURCE_DIR}/rotate_msa.cc
    ${LIBYUV_SOURCE_DIR}/rotate_win.cc
    ${LIBYUV_SOURCE_DIR}/row_any.cc
    ${LIBYUV_SOURCE_DIR}/row_common.cc
    ${LIBYUV_SOURCE_DIR}/row_gcc.cc
    ${LIBYUV_SOURCE_DIR}/row_lasx.cc
    ${LIBYUV_SOURCE_DIR}/row_lsx.cc
    ${LIBYUV_SOURCE_DIR}/row_msa.cc
    ${LIBYUV_SOURCE_DIR}/row_rvv.cc
    ${LIBYUV_SOURCE_DIR}/row_win.cc
    ${LIBYUV_SOURCE_DIR}/scale_any.cc
    ${LIBYUV_SOURCE_DIR}/scale_argb.cc
    ${LIBYUV_SOURCE_DIR}/scale.cc
    ${LIBYUV_SOURCE_DIR}/scale_common.cc
    ${LIBYUV_SOURCE_DIR}/scale_gcc.cc
    ${LIBYUV_SOURCE_DIR}/scale_lsx.cc
    ${LIBYUV_SOURCE_DIR}/scale_msa.cc
    ${LIBYUV_SOURCE_DIR}/scale_rgb.cc
    ${LIBYUV_SOURCE_DIR}/scale_rvv.cc
    ${LIBYUV_SOURCE_DIR}/scale_uv.cc
    ${LIBYUV_SOURCE_DIR}/scale_win.cc
    ${LIBYUV_SOURCE_DIR}/video_common.cc
)

include_directories(BEFORE ${LIBYUV_INCLUDE_DIR})

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4310 /wd4244")
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" arch_lowercase)

    if(LIBYUV_ENABLE_NEON)
        if(arch_lowercase MATCHES "^arm" AND NOT arch_lowercase STREQUAL "arm64")
            # Enable Arm Neon kernels.
            add_definitions(-DLIBYUV_NEON=1)
            add_library(${LIBYUV}_neon OBJECT
                ${LIBYUV_SOURCE_DIR}/compare_neon.cc
                ${LIBYUV_SOURCE_DIR}/rotate_neon.cc
                ${LIBYUV_SOURCE_DIR}/row_neon.cc
                ${LIBYUV_SOURCE_DIR}/scale_neon.cc)
            target_compile_options(${LIBYUV}_neon PRIVATE -mfpu=neon)
        endif()
    endif()

    if(arch_lowercase STREQUAL "aarch64" OR arch_lowercase STREQUAL "arm64")
        if(LIBYUV_ENABLE_NEON)
            # Enable AArch64 Neon dot-product and i8mm kernels.
            add_library(${LIBYUV}_neon64 OBJECT
                ${LIBYUV_SOURCE_DIR}/compare_neon64.cc
                ${LIBYUV_SOURCE_DIR}/rotate_neon64.cc
                ${LIBYUV_SOURCE_DIR}/row_neon64.cc
                ${LIBYUV_SOURCE_DIR}/scale_neon64.cc)
            target_compile_options(${LIBYUV}_neon64 PRIVATE -march=armv8-a+dotprod+i8mm)
        endif()

        if(LIBYUV_ENABLE_SVE)
            # Enable AArch64 SVE kernels.
            add_library(${LIBYUV}_sve OBJECT
                ${LIBYUV_SOURCE_DIR}/row_sve.cc)
            target_compile_options(${LIBYUV}_sve PRIVATE -march=armv9-a+sve2)
        endif()

        if(LIBYUV_ENABLE_SME)
            set(OLD_CMAKE_REQURED_FLAGS ${CMAKE_REQUIRED_FLAGS})
            set(OLD_CMAKE_TRY_COMPILE_TARGET_TYPE ${CMAKE_TRY_COMPILE_TARGET_TYPE})
            set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=armv9-a+sme")
            set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

            # Check whether the compiler can compile SME functions; this fails
            # with Clang for Windows.
            check_c_source_compiles("__arm_locally_streaming void func(void) { } int main(void) { return 0; }" CAN_COMPILE_SME)
            set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQURED_FLAGS})
            set(CMAKE_TRY_COMPILE_TARGET_TYPE ${OLD_CMAKE_TRY_COMPILE_TARGET_TYPE})

            if(CAN_COMPILE_SME)
                # Enable AArch64 SME kernels.
                add_library(${LIBYUV}_sme OBJECT
                    ${LIBYUV_SOURCE_DIR}/rotate_sme.cc)
                target_compile_options(${LIBYUV}_sme PRIVATE -march=armv9-a+sme)
            else()
                add_definitions(-DLIBYUV_DISABLE_SME)
            endif()
        else()
            add_definitions(-DLIBYUV_DISABLE_SME)
        endif()
    endif()
endif()

# this creates the static library (.a)
add_library(${LIBYUV} STATIC ${LIBYUV_COMMON_SOURCE_FILES})

# create the .deb and .rpm packages using cpack
# include ( CM_linux_packages.cmake )
