# toolchain for aarch64-linux-gnu from obs

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-gnu)

set(CMAKE_ASM_COMPILER clang)
set(CMAKE_ASM_COMPILER_TARGET aarch64-linux-gnu)
