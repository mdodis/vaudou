#ifndef VD_MEMORY_H
#define VD_MEMORY_H

#include "common.h"

#define VD_PROC_ALLOC(name) umm name(umm ptr, size_t prevsize, size_t newsize, void *c)
typedef VD_PROC_ALLOC(VD_ProcAlloc);

typedef struct {
    VD_ProcAlloc *proc_alloc;
    void         *c;
} VD_Allocator;

VD_Allocator *vd_memory_get_system_allocator(void);

VD_INLINE umm vd_realloc(VD_Allocator *allocator, umm ptr, size_t prevsize, size_t newsize)
{
    return allocator->proc_alloc(ptr, prevsize, newsize, allocator->c);
}

VD_INLINE umm vd_free(VD_Allocator *allocator, umm ptr, size_t size)
{
    return allocator->proc_alloc(ptr, size, 0, allocator->c);
}

#endif