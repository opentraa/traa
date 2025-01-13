#include <stdlib.h>

// DO NOT IMPLEMENT MAIN FUNCTION, UNLESS YOU WANT TO EXCUTE TESTS BY YOURSELF
// int main(int argc, char **argv) {
//   return EXIT_SUCCESS;
// }

#include "base/system/metrics.h"

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
    traa_init_test(void) {

#if defined(TRAA_METRICS_ENABLED)
  traa::base::metrics::enable();
#endif

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
__declspec(allocate(".CRT$XCU")) int (*_traa_init)(void) = traa_init_test;
#endif