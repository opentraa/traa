# Enable address sanitizer (gcc/clang only)
# See:
# https://github.com/google/sanitizers/wiki/AddressSanitizer
# The tool works on x86, ARM, MIPS (both 32- and 64-bit versions of all architectures), PowerPC64.
# The supported operation systems are Linux, Darwin (OS X and iOS Simulator), FreeBSD, Android:
# OS	        x86	x86_64	ARM	ARM64	MIPS	MIPS64	PowerPC	PowerPC64
# Linux	        yes	yes		yes	yes	    yes	    yes
# OS X	        yes	yes
# iOS Simulator	yes	yes
# FreeBSD	    yes	yes
# Android	    yes	yes	    yes	yes
# Windows 10    yes	yes
function(traa_enable_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|MSVC")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang/msvc")
    endif()

    message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address)

    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # they recommand to use /DEBUG to see more details of the stack trace
        target_link_options(${target_name} PRIVATE /DEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
        target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
        target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
        target_link_libraries(${target_name} PRIVATE -fsanitize=address)
    endif()
endfunction()