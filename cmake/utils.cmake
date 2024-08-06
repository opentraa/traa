# Enable address sanitizer (gcc/clang only)
function(traa_enable_sanitizer target_name)
    # See:
    # https://github.com/google/sanitizers/wiki/AddressSanitizer
    # OS	         x86	x86_64	ARM	ARM64	MIPS	MIPS64	PowerPC	PowerPC64
    # Linux	         yes	yes			        yes	     yes	yes	      yes
    # OS X	         yes	yes						
    # iOS Simulator	 yes	yes						
    # FreeBSD	     yes	yes						
    # Android	     yes	yes	    yes	 yes				
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif ()
    message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined)
endfunction()