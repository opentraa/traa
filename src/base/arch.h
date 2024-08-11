#ifndef TRAA_BASE_ARCH_H_
#define TRAA_BASE_ARCH_H_

#if defined(_M_X64) || defined(__x86_64__)
#define TRAA_ARCH_X86_FAMILY
#define TRAA_ARCH_X86_64
#define TRAA_ARCH_64_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(_M_ARM64) || defined(__aarch64__)
#define TRAA_ARCH_ARM_FAMILY
#define TRAA_ARCH_64_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(_M_IX86) || defined(__i386__)
#define TRAA_ARCH_X86_FAMILY
#define TRAA_ARCH_X86
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(_M_ARM) || defined(__ARMEL__)
#define TRAA_ARCH_ARM_FAMILY
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__MIPSEL__) || defined(__MIPSEB__)
#define TRAA_ARCH_MIPS_FAMILY
#if defined(__LP64__)
#define TRAA_ARCH_64_BITS
#else
#define TRAA_ARCH_32_BITS
#endif
#if defined(__MIPSEL__)
#define TRAA_ARCH_LITTLE_ENDIAN
#else
#define TRAA_ARCH_BIG_ENDIAN
#endif
#elif defined(__PPC__)
#if defined(__PPC64__)
#define TRAA_ARCH_64_BITS
#else
#define TRAA_ARCH_32_BITS
#endif
#if defined(__LITTLE_ENDIAN__)
#define TRAA_ARCH_LITTLE_ENDIAN
#else
#define TRAA_ARCH_BIG_ENDIAN
#endif
#elif defined(__sparc) || defined(__sparc__)
#if __SIZEOF_LONG__ == 8
#define TRAA_ARCH_64_BITS
#else
#define TRAA_ARCH_32_BITS
#endif
#define TRAA_ARCH_BIG_ENDIAN
#elif defined(__riscv) && __riscv_xlen == 64
#define TRAA_ARCH_64_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__riscv) && __riscv_xlen == 32
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__loongarch32)
#define TRAA_ARCH_LOONG_FAMILY
#define TRAA_ARCH_LOONG32
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__loongarch64)
#define TRAA_ARCH_LOONG_FAMILY
#define TRAA_ARCH_LOONG64
#define TRAA_ARCH_64_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__pnacl__)
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#elif defined(__EMSCRIPTEN__)
#define TRAA_ARCH_32_BITS
#define TRAA_ARCH_LITTLE_ENDIAN
#else
#error Please add support for your architecture in traa/arch.h
#endif
#if !(defined(TRAA_ARCH_LITTLE_ENDIAN) ^ defined(TRAA_ARCH_BIG_ENDIAN))
#error Define either TRAA_ARCH_LITTLE_ENDIAN or TRAA_ARCH_BIG_ENDIAN
#endif

#endif // TRAA_BASE_ARCH_H_