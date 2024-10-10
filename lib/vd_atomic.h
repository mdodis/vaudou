// vd_atomic.h - A C atomics library
// 
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
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
#define load_ptr              vd_atomic_load_ptr
#define inc_and_fetch32       vd_atomic_inc_and_fetch32
#define inc_and_fetchu32      vd_atomic_inc_and_fetchu32
#define add_and_fetch32       vd_atomic_add_and_fetch32
#define fetch_and_add64       vd_atomic_fetch_and_add64
#endif

#include <stdint.h>

#if VD_ATOMIC_PLATFORM_WINDOWS
#include <intrin.h>
#define MEMORY_BARRIER() do { _ReadBarrier(); _WriteBarrier(); MemoryBarrier(); } while(0)
#else
#define MEMORY_BARRIER() do {__sync_synchronize(); asm volatile("mfence": : :"memory"); } while(0)
#endif

static inline int32_t vd_atomic_compare_and_swap32(volatile int32_t *ptr, int32_t new_value, int32_t expected) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedCompareExchange(ptr, new_value, expected);
#else
    return __sync_val_compare_and_swap(ptr, expected, new_value);
#endif
}

static inline uint32_t vd_atomic_compare_and_swapu32(volatile uint32_t *ptr, uint32_t new_value, uint32_t expected) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedCompareExchange(ptr, new_value, expected);
#else
    return __sync_val_compare_and_swap(ptr, expected, new_value);
#endif
}

static inline int64_t vd_atomic_compare_and_swap64(volatile int64_t *ptr, int64_t new_value, int64_t expected) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return InterlockedCompareExchange64(ptr, new_value, expected);
#else
    return __sync_val_compare_and_swap(ptr, expected, new_value);
#endif
}

static inline void *vd_atomic_compare_and_swap_ptr(void *volatile *ptr, void *new_value, void *expected) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedCompareExchangePointer(ptr, new_value, expected);
#else
    return __sync_val_compare_and_swap(ptr, expected, new_value);
#endif
}

static inline void *vd_atomic_load_ptr(void *volatile ptr)
{
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedCompareExchangePointer(ptr, 0, 0);
#else
#endif
}

static inline int32_t vd_atomic_inc_and_fetch32(volatile int32_t *addend) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedIncrement(addend);
#else
    return __sync_add_and_fetch(addend, 1);
#endif
}

static inline uint32_t vd_atomic_inc_and_fetchu32(volatile uint32_t *addend) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedIncrement(addend);
#else
    return __sync_add_and_fetch(addend, 1);
#endif
}

static inline int32_t vd_atomic_add_and_fetch32(volatile int32_t *addend, int32_t value) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _interlockedadd(addend, value);
#else
    return __sync_add_and_fetch(addend, value);
#endif
}

static inline int32_t vd_atomic_add_and_fetch64(volatile int64_t *addend, int64_t value) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return InterlockedAdd64((volatile long long *)addend, value);
#else
    return __sync_add_and_fetch(addend, value);
#endif
}

static inline int64_t vd_atomic_fetch_and_add64(volatile int64_t *addend, int64_t value) {
#if VD_ATOMIC_PLATFORM_WINDOWS
    return _InterlockedExchangeAdd64(addend, value);
#else
    return __sync_fetch_and_add(addend, value);
#endif
}

#endif // !VD_ATOMIC_H