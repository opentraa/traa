#include "base/logger.h"
#include "base/platform.h"
#include "base/thread/task_queue.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Define the TRAA_DEBUG_LOG macro to print debug log messages.
 *
 * The TRAA_DEBUG_LOG macro is used to print debug log messages to the console and is only defined
 * if the __DEBUG macro is defined.
 */
#ifdef TRAA_DEBUG
#define TRAA_DEBUG_LOG(...) printf(__VA_ARGS__)
#else
#define TRAA_DEBUG_LOG(...) void()
#endif

/**
 * @brief Finalize the traa module.
 *
 * This function is called when the traa module is unloaded automatically by the operating system
 * after all other internal code is executed, but before the CRT is deinitialized.
 */
static void
#if defined(__GNUC__)
    __attribute__((destructor, used))
#elif defined(_MSC_VER)
// do nothing see atexit(traa_fini) in traa_init below
#endif
    traa_fini(void) {
  TRAA_DEBUG_LOG("traa_fini started\r\n");

  // DO NOT DO ANYTHING THAT DEPENDS ON OTHER MODULES OR ANY ROGUE BEHAVIOR

  TRAA_DEBUG_LOG("traa_fini finished\r\n");
}

/**
 * @brief Initialize the traa module.
 *
 * This function is called when the traa module is loaded automatically by the operating system
 * before any other internal code is executed, but after the CRT is initialized.
 */
static
#if defined(__GNUC__)
    void
#elif defined(_MSC_VER)
    int
#else
#error "Unsupported compiler"
#endif
#if defined(__GNUC__)
    __attribute__((constructor, used))
#endif
    traa_init(void) {
  TRAA_DEBUG_LOG("traa_init started\r\n");

  // DO NOT DO ANYTHING THAT DEPENDS ON OTHER MODULES OR ANY ROGUE BEHAVIOR

#if defined(_MSC_VER)
  atexit(traa_fini);
#endif

  TRAA_DEBUG_LOG("traa_init finished\r\n");

#if defined(_MSC_VER)
  return 0;
#endif
}

#if defined(_MSC_VER)
/**
 * About the section '.CRT$XCU':
 *
 * https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization?view=msvc-170
 *
 * '.CRT$XCU' is a section that is used to specify the initialization function for the traa
 * module.
 *
 * To view more details about the sections used by the CRT, view the crt
 * file.(crt\src\vcruntime\internal_shared.h)
 */
#pragma section(".CRT$XCU", long, read)
__declspec(allocate(".CRT$XCU")) int (*_traa_init)(void) = traa_init;
#endif
