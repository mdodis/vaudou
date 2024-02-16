#ifndef VD_ARENA_H
#define VD_ARENA_H
#include "common.h"
#include "allocator.h"
#include <string.h>

typedef struct {
    umm           data;
    umm           begin;
    umm           end;
    VD_Allocator *allocator;
} VD_Arena;

#define VD_ARENA_ALLOC(arena, size) (vd_arena_alloc(arena, size, 8))
#define VD_ARENA_ALLOC_ARRAY(arena, count, size) (vd_arena_alloc(arena, count * size, 8))
#define VD_ARENA_ALLOC_STRUCT(arena, type) ((type*)vd_arena_alloc(arena, sizeof(type), 8))

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

VD_INLINE void vd_arena_reset(VD_Arena *arena)
{
    arena->begin = arena->data;
}

VD_INLINE void vd_arena_free(VD_Arena *arena)
{
    vd_free(arena->allocator, arena->data, arena->end - arena->data);
}

#if VD_ABBREVIATIONS
#define Arena VD_Arena
#define arena_new vd_arena_new
#define arena_alloc VD_ARENA_ALLOC
#define arena_alloc_array VD_ARENA_ALLOC_ARRAY
#define arena_alloc_struct VD_ARENA_ALLOC_STRUCT
#define arena_reset vd_arena_reset
#define arena_free vd_arena_free
#endif

#endif