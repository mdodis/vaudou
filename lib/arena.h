#ifndef VD_ARENA_H
#define VD_ARENA_H
#include "vd_common.h"
#include <string.h>
#include "vd_atomic.h"

typedef struct {
    umm           data;
    umm           begin;
    umm           end;
    VD_Allocator *allocator;
} VD_Arena;

#define VD_ARENA_ALLOC(arena, size) (vd_arena_alloc(arena, size, 8))
#define VD_ARENA_ALLOC_T(arena, size) (vd_arena_alloc_t(arena, size, 8))
#define VD_ARENA_ALLOC_ARRAY(arena, count, size) (vd_arena_alloc(arena, count * size, 8))
#define VD_ARENA_ALLOC_ARRAY_T(arena, count, size) (vd_arena_alloc_t(arena, count * size, 8))
#define VD_ARENA_ALLOC_STRUCT_T(arena, type) ((type*)vd_arena_alloc_t(arena, sizeof(type), 8))

VD_INLINE VD_Arena vd_arena_new(ptrdiff_t size, VD_Allocator *allocator)
{
    umm data = vd_realloc(allocator, 0, 0, (size_t)size);
    VD_Arena arena = {data, data, ((uintptr_t)data + size), allocator};
    return arena;
}

VD_INLINE void *vd_arena_alloc(VD_Arena *arena, ptrdiff_t size, ptrdiff_t align)
{
    ptrdiff_t padding = -(uintptr_t)arena->begin & (align - 1);
    ptrdiff_t available = arena->end - arena->begin - padding;

    if (available < 0) {
        return 0;
    }

    void *p = (void*)(arena->begin + padding);
    arena->begin += padding + size;
    return memset(p, 0, size);
}

VD_INLINE void *vd_arena_alloc_t(VD_Arena *arena, ptrdiff_t size, ptrdiff_t align)
{
    ptrdiff_t padding = -(uintptr_t)arena->begin & (align - 1);
    ptrdiff_t available = arena->end - arena->begin - padding;

    if (available < 0) {
        return 0;
    }

    umm p = (umm)vd_atomic_fetch_and_adduptr(&arena->begin, padding + size);


    p += padding;

    if (p + size > arena->end) {
        return 0;
    }

    return memset((void*)p, 0, size);
}

VD_INLINE void vd_arena_reset(VD_Arena *arena)
{
    arena->begin = arena->data;
}

VD_INLINE void vd_arena_free(VD_Arena *arena)
{
    vd_free(arena->allocator, arena->data, arena->end - arena->data);
}

VD_INLINE void vd_arena_get_stats(VD_Arena *arena, u64 *used, u64 *total)
{
    *used = arena->begin - arena->data;
    *total = arena->end - arena->data;
}

VD_PROC_ALLOC(vd_arena_proc_alloc);

#if VD_ABBREVIATIONS
#define Arena VD_Arena
#define arena_new vd_arena_new
#define arena_alloc VD_ARENA_ALLOC
#define arena_alloc_array VD_ARENA_ALLOC_ARRAY
#define arena_alloc_struct VD_ARENA_ALLOC_STRUCT
#define arena_alloc_t VD_ARENA_ALLOC_T
#define arena_alloc_array_t VD_ARENA_ALLOC_ARRAY_T
#define arena_alloc_struct_t VD_ARENA_ALLOC_STRUCT_T
#define arena_reset vd_arena_reset
#define arena_free vd_arena_free
#endif

#endif
