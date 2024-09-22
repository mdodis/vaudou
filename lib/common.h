#ifndef VD_COMMON_H
#define VD_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ----HOST COMPILER----------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
#define VD_HOST_COMPILER_CLANG 1
#endif

#if defined(_MSC_VER)
#define VD_HOST_COMPILER_MSVC 1
#endif

#ifndef VD_HOST_COMPILER_CLANG
#define VD_HOST_COMPILER_CLANG 0
#endif

#ifndef VD_HOST_COMPILER_MSVC
#define VD_HOST_COMPILER_MSVC 0
#endif

#if defined(__cplusplus)
#define VD_CPP 1
#endif

#ifndef VD_CPP
#define VD_CPP 0
#endif

#if VD_CPP
#define VD_C 0
#else
#define VD_C 1
#endif

/* ----PLATFORM---------------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
#define VD_PLATFORM_WINDOWS 1
#endif

#if defined(__linux__)
#define VD_PLATFORM_LINUX 1
#endif

#if defined(__APPLE__)
#define VD_PLATFORM_MACOS 1
#endif

#ifndef VD_PLATFORM_WINDOWS
#define VD_PLATFORM_WINDOWS 0
#endif

#ifndef VD_PLATFORM_LINUX
#define VD_PLATFORM_LINUX 0
#endif

#ifndef VD_PLATFORM_MACOS
#define VD_PLATFORM_MACOS 0
#endif

/* ----TYPES------------------------------------------------------------------------------------- */
typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;
typedef unsigned char uchar;
typedef uintptr_t     umm;
typedef char         *cstr;
typedef float         f32;
typedef double        f64;

#define VD_FALSE 0
#define VD_TRUE  1

#if VD_C
typedef char VD_bool;
#else
typedef bool VD_bool;
#endif

/* ----ABBREVIATIONS----------------------------------------------------------------------------- */
#ifndef VD_ABBREVIATIONS
#define VD_ABBREVIATIONS 0
#endif

#ifndef VD_INTERNAL_SOURCE_FILE
#define VD_INTERNAL_SOURCE_FILE 0
#endif

#if VD_INTERNAL_SOURCE_FILE
#ifdef VD_ABBREVIATIONS
#undef VD_ABBREVIATIONS
#endif
#define VD_ABBREVIATIONS 1
#endif

/* ----BUILD CONFIGURATION----------------------------------------------------------------------- */
#ifndef VD_BUILD_DEBUG
#define VD_BUILD_DEBUG 0
#endif

#if VD_BUILD_DEBUG
#define VD_DEBUG 1
#endif

#ifndef VD_DEBUG
#define VD_DEBUG 0
#endif

#if VD_DEBUG
#define VD_SANITY_CHECKS 1
#else
#define VD_SANITY_CHECKS 0
#endif



/* ----KEYWORDS---------------------------------------------------------------------------------- */
#if VD_HOST_COMPILER_CLANG
#define VD_INLINE __attribute__((always_inline)) inline
#elif VD_HOST_COMPILER_MSVC
#define VD_INLINE __forceinline
#else
#define VD_INLINE static inline
#endif

/**
 * @brief Indicates this BOOLEAN expression should never be true.
 * @param x The expression.
 * @return VD_FALSE when VD_SANITY_CHECKS is 1, otherwise the value of x.
 */
#define VD_IMPOSSIBLE(x) (VD_FALSE)
#if VD_SANITY_CHECKS
#undef VD_IMPOSSIBLE
#define VD_IMPOSSIBLE(x) (x)
#endif

/**
 * @brief Indicates this function should be not be exposed as a symbol.
 */
#define VD_INTERNAL static

/**
 * @brief Indicates this parameter is unused.
 */
#define VD_UNUSED(x) (void)(x)

/* ----LIMITS------------------------------------------------------------------------------------ */
#define VD_U8_MAX    UINT8_MAX
#define VD_U16_MAX   UINT16_MAX
#define VD_U32_MAX   UINT32_MAX
#define VD_U64_MAX   UINT64_MAX
#define VD_I8_MAX    INT8_MAX
#define VD_I16_MAX   INT16_MAX
#define VD_I32_MAX   INT32_MAX
#define VD_I64_MAX   INT64_MAX
#define VD_SIZET_MAX SIZE_MAX

/* ----SIZES------------------------------------------------------------------------------------ */
#define VD_KILOBYTES(x) x * 1024
#define VD_MEGABYTES(x) VD_KILOBYTES(x) * 1024

#define VD_ARRAY_COUNT(s) (sizeof(s) / sizeof(*s))

/* ----LOCAL ABBREVIATIONS----------------------------------------------------------------------- */
#if VD_ABBREVIATIONS
#define _impossible(x) VD_IMPOSSIBLE(x)
#define _inline VD_INLINE
#define _internal VD_INTERNAL
#define _unused(x) VD_UNUSED(x)

#define ARRAY_COUNT VD_ARRAY_COUNT
#endif

#endif // VD_COMMON_H