# toolchain for x86_64-linux-gnu from obs

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(CROSS STREQUAL "")
  set(CROSS x86_64-linux-gnu-)
endif()

if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER ${CROSS}gcc)
endif()

if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${CROSS}g++)
endif()

if(NOT CMAKE_ASM_COMPILER)
  set(CMAKE_ASM_COMPILER ${CROSS}as)
endif()
