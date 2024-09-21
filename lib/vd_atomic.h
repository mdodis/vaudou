#ifndef VD_ATOMIC_H
#define VD_ATOMIC_H

#if defined(_WIN32) || defined(_WIN64)
#define VD_ATOMIC_PLATFORM_WINDOWS 1
#endif

#if defined(__linux__)
#define VD_ATOMIC_PLATFORM_LINUX 1
#endif

#if defined(__APPLE__)
#define VD_ATOMIC_PLATFORM_MACOS 1
#endif

#ifndef VD_ATOMIC_PLATFORM_WINDOWS
#define VD_ATOMIC_PLATFORM_WINDOWS 0
#endif

#ifndef VD_ATOMIC_PLATFORM_LINUX
#define VD_ATOMIC_PLATFORM_LINUX 0
#endif

#ifndef VD_ATOMIC_PLATFORM_MACOS
#define VD_ATOMIC_PLATFORM_MACOS 0
#endif

#ifndef VD_ATOMIC_ABBREVIATIONS
#define VD_ATOMIC_ABBREVIATIONS 1
#endif

#if VD_ATOMIC_ABBREVIATIONS
#define compare_and_swap32    vd_atomic_compare_and_swap32
#define compare_and_swapu32   vd_atomic_compare_and_swapu32
#define compare_and_swap64    vd_atomic_compare_and_swap64
#define compare_and_swap_ptr  vd_atomic_compare_and_swap_ptr
#define inc_and_fetch32       vd_atomic_inc_and_fetch32
#define inc_and_fetchu32      vd_atomic_inc_and_fetchu32
#define add_and_fetch32       vd_atomic_add_and_fetch32
#define fetch_and_add64       vd_atomic_fetch_and_add64
#endif

#include <stdint.h>

#if VD_ATOMIC_PLATFORM_WINDOWS
#include <intrin.h>
#define MEMORY_BARRIER() do { _ReadBarrier(); _WriteBarrier(); MemoryBarrier(); } while(0)

_inline int32_t vd_atomic_compare_and_swap32(volatile int32_t *ptr, int32_t new_value, int32_t expected) {
    return _InterlockedCompareExchange(ptr, new_value, expected);
}

_inline uint32_t vd_atomic_compare_and_swapu32(volatile uint32_t *ptr, uint32_t new_value, uint32_t expected) {
    return _InterlockedCompareExchange(ptr, new_value, expected);
}

_inline int64_t vd_atomic_compare_and_swap64(volatile int64_t *ptr, int64_t new_value, int64_t expected) {
    return InterlockedCompareExchange64(ptr, new_value, expected);
}

_inline void *vd_atomic_compare_and_swap_ptr(void *volatile *ptr, void *new_value, void *expected) {
    return _InterlockedCompareExchangePointer(ptr, new_value, expected);
}

_inline int32_t vd_atomic_inc_and_fetch32(volatile int32_t *addend) {
    return _InterlockedIncrement(addend);
}

_inline uint32_t vd_atomic_inc_and_fetchu32(volatile uint32_t *addend) {
    return _InterlockedIncrement(addend);
}

_inline int32_t vd_atomic_add_and_fetch32(volatile int32_t *addend, int32_t value) {
    return _interlockedadd(addend, value);
}

_inline int32_t vd_atomic_add_and_fetch64(volatile int64_t *addend, int64_t value) {
    return InterlockedAdd64((volatile long long *)addend, value);
}

_inline int64_t vd_atomic_fetch_and_add64(volatile int64_t *addend, int64_t value) {
    return _InterlockedExchangeAdd64(addend, value);
}

#else
#define MEMORY_BARRIER() do {__sync_synchronize(); asm volatile("mfence": : :"memory"); } while(0)

_inline int32_t vd_atomic_compare_and_swap32(volatile int32_t *ptr, int32_t new_value, int32_t expected) {
    return __sync_val_compare_and_swap(ptr, expected, new_value);
}

_inline uint32_t vd_atomic_compare_and_swapu32(volatile uint32_t *ptr, uint32_t new_value, uint32_t expected) {
    return __sync_val_compare_and_swap(ptr, expected, new_value);
}

_inline int64_t vd_atomic_compare_and_swap64(volatile int64_t *ptr, int64_t new_value, int64_t expected) {
    return __sync_val_compare_and_swap(ptr, expected, new_value);
}

_inline void *vd_atomic_compare_and_swap_ptr(void *volatile *ptr, void *new_value, void *expected) {
    return __sync_val_compare_and_swap(ptr, expected, new_value);
}

_inline int32_t vd_atomic_inc_and_fetch32(volatile int32_t *addend) {
    return __sync_add_and_fetch(addend, 1);
}

_inline uint32_t vd_atomic_inc_and_fetchu32(volatile uint32_t *addend) {
    return __sync_add_and_fetch(addend, 1);
}

_inline int32_t vd_atomic_add_and_fetch32(volatile int32_t *addend, int32_t value) {
    return __sync_add_and_fetch(addend, value);
}
#endif

#endif // !VD_ATOMIC_H