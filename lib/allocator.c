#define VD_INTERNAL_SOURCE_FILE 1
#include "allocator.h"
#include "common.h"
#include <stdlib.h>

_internal VD_PROC_ALLOC(sys_alloc);
static VD_Allocator System_Allocator = {
    .proc_alloc = sys_alloc,
    .c = 0,
};

VD_Allocator *vd_memory_get_system_allocator(void)
{
    return &System_Allocator;
}


_internal VD_PROC_ALLOC(sys_alloc)
{
    _unused(c);

    if (newsize == 0) {
        free(ptr);
        return 0;
    } else {
        return realloc(ptr, newsize);
    }
}