#ifndef TRAA_EXPORT_H
#define TRAA_EXPORT_H

#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define TRAA_API extern "C" __global
#define TRAA_CALL
#elif defined __GNUC__
#define TRAA_API extern "C" __attribute__((visibility("default")))
#define TRAA_CALL
#elif defined(_MSC_VER)
#if defined(TRAA_EXPORT)
#define TRAA_API extern "C" __declspec(dllexport)
#else
#define TRAA_API extern "C" __declspec(dllimport)
#endif
#define TRAA_CALL __cdecl
#else
#define TRAA_API /* unknown compiler */
#endif

#if defined(TRAA_UNIT_TEST)
#if defined(TRAA_API)
#undef TRAA_API
#endif
#define TRAA_API
#endif

#if defined(TRAA_API_UNDEF)
#if defined(TRAA_API)
#undef TRAA_API
#define TRAA_API
#endif // defined(TRAA_API)

#if defined(TRAA_CALL)
#undef TRAA_CALL
#define TRAA_CALL
#endif // defined(TRAA_CALL)
#endif // defined(TRAA_API_UNDEF)

#if (defined(__GNUC__) && !defined(__LCC__)) || defined(__clang__)
#define TRAA_DEPRECATED __attribute__((deprecated))
#elif FMT_MSC_VER
#define TRAA_DEPRECATED __declspec(deprecated)
#else
#define TRAA_DEPRECATED /* deprecated */
#endif

#endif // TRAA_EXPORT_H