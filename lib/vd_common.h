#ifndef VD_COMMON_H
#define VD_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

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
 * @brief Indicates this function should not be exposed as a symbol.
 */
#define VD_INTERNAL static

/**
 * @brief Indicates this parameter is unused.
 */
#define VD_UNUSED(x) (void)(x)

#define PtrTo(x) x*

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

/* ----ALLOCATORS-------------------------------------------------------------------------------- */
#define VD_PROC_ALLOC(name) umm name(umm ptr, size_t prevsize, size_t newsize, void *c)
typedef VD_PROC_ALLOC(VD_ProcAlloc);

typedef struct {
    VD_ProcAlloc *proc_alloc;
    void         *c;
} VD_Allocator;

VD_INTERNAL VD_PROC_ALLOC(sys_alloc)
{
    VD_UNUSED(c);

    if (newsize == 0) {
        free((void*)ptr);
        return 0;
    } else {
        return (umm)realloc((void*)ptr, newsize);
    }
}

static VD_Allocator System_Allocator = {
    .proc_alloc = sys_alloc,
    .c = 0,
};

static VD_INLINE VD_Allocator *vd_memory_get_system_allocator(void)
{
    return &System_Allocator;
}

VD_INLINE void *vd_malloc(VD_Allocator *allocator, size_t size)
{
    return (void*)allocator->proc_alloc(0, 0, size, allocator->c);
}

VD_INLINE umm vd_realloc(VD_Allocator *allocator, umm ptr, size_t prevsize, size_t newsize)
{
    return allocator->proc_alloc(ptr, prevsize, newsize, allocator->c);
}

VD_INLINE umm vd_free(VD_Allocator *allocator, umm ptr, size_t size)
{
    return allocator->proc_alloc(ptr, size, 0, allocator->c);
}

#define VD_ALLOC_ARRAY(alc, s, n) ((s*)vd_malloc(alc, sizeof(s) * n))

/* ----COLORS------------------------------------------------------------------------------------ */

VD_INLINE u32 vd_pack_unorm_r8g8b8a8(float v[4])
{
    return (u32)(
        ((u8)(v[0] * 255.f)) <<  0 |
        ((u8)(v[1] * 255.f)) <<  8 |
        ((u8)(v[2] * 255.f)) << 16 |
        ((u8)(v[3] * 255.f)) << 24
    );
}

/* ----LOCAL ABBREVIATIONS----------------------------------------------------------------------- */
#if VD_ABBREVIATIONS
#define _impossible(x) VD_IMPOSSIBLE(x)
#define _inline VD_INLINE
#define _internal VD_INTERNAL
#define _unused(x) VD_UNUSED(x)

#define ARRAY_COUNT VD_ARRAY_COUNT
#endif

#ifndef VD_CUSTOM_PREFIXES
#define VD_(x) x
#define VD(x)  x
#define vd(x)  x
#endif

#endif // VD_COMMON_H
