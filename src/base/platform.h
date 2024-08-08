#ifndef TRAA_BASE_PLATFORM_H_
#define TRAA_BASE_PLATFORM_H_

#ifdef __CYGWIN__
#define TRAA_OS_CYGWIN 1
#elif defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
#define TRAA_OS_WINDOWS_MINGW 1
#define TRAA_OS_WINDOWS 1
#elif defined _WIN32
#define TRAA_OS_WINDOWS 1
#ifdef _WIN32_WCE
#define TRAA_OS_WINDOWS_MOBILE 1
#elif defined(WINAPI_FAMILY)
#include <winapifamily.h>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define TRAA_OS_WINDOWS_DESKTOP 1
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#define TRAA_OS_WINDOWS_PHONE 1
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define TRAA_OS_WINDOWS_RT 1
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_TV_TITLE)
#define TRAA_OS_WINDOWS_PHONE 1
#define TRAA_OS_WINDOWS_TV_TITLE 1
#elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_GAMES)
#define TRAA_OS_WINDOWS_GAMES 1
#else
// WINAPI_FAMILY defined but no known partition matched.
// Default to desktop.
#define TRAA_OS_WINDOWS_DESKTOP 1
#endif
#else
#define TRAA_OS_WINDOWS_DESKTOP 1
#endif // _WIN32_WCE
#elif defined __OS2__
#define TRAA_OS_OS2 1
#elif defined __APPLE__
#define TRAA_OS_MAC 1
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define TRAA_OS_IOS 1
#endif
#elif defined __DragonFly__
#define TRAA_OS_DRAGONFLY 1
#elif defined __FreeBSD__
#define TRAA_OS_FREEBSD 1
#elif defined __Fuchsia__
#define TRAA_OS_FUCHSIA 1
#elif defined(__GNU__)
#define TRAA_OS_GNU_HURD 1
#elif defined(__GLIBC__) && defined(__FreeBSD_kernel__)
#define TRAA_OS_GNU_KFREEBSD 1
#elif defined __linux__
#define TRAA_OS_LINUX 1
#if defined __ANDROID__
#define TRAA_OS_LINUX_ANDROID 1
#endif
#elif defined __MVS__
#define TRAA_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
#define TRAA_OS_SOLARIS 1
#elif defined(_AIX)
#define TRAA_OS_AIX 1
#elif defined(__hpux)
#define TRAA_OS_HPUX 1
#elif defined __native_client__
#define TRAA_OS_NACL 1
#elif defined __NetBSD__
#define TRAA_OS_NETBSD 1
#elif defined __OpenBSD__
#define TRAA_OS_OPENBSD 1
#elif defined __QNX__
#define TRAA_OS_QNX 1
#elif defined(__HAIKU__)
#define TRAA_OS_HAIKU 1
#elif defined ESP8266
#define TRAA_OS_ESP8266 1
#elif defined ESP32
#define TRAA_OS_ESP32 1
#elif defined(__XTENSA__)
#define TRAA_OS_XTENSA 1
#elif defined(__hexagon__)
#define TRAA_OS_QURT 1
#elif defined(CPU_QN9090) || defined(CPU_QN9090HN)
#define TRAA_OS_NXP_QN9090 1
#elif defined(NRF52)
#define TRAA_OS_NRF52 1
#endif // __CYGWIN__

#if !defined(TRAA_OS_WINDOWS) && !defined(TRAA_OS_LINUX) && !defined(TRAA_OS_MAC) &&               \
    !defined(TRAA_OS_IOS) && !defined(TRAA_OS_ANDROID)
#error "Do not support current target system!"
#endif

#endif // TRAA_BASE_PLATFORM_H_