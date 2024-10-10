#include "arena.h"
VD_PROC_ALLOC(vd_arena_proc_alloc)
{
    VD_Arena *arena = (VD_Arena*)c;
    if (newsize == 0) {
        return 0;
    }

    umm nptr = (umm)vd_arena_alloc(arena, newsize, 8);
    if (nptr == 0) {
        return 0;
    }

    if (ptr != 0) {
        memcpy((void*)nptr, (void*)ptr, prevsize);
    }

    return nptr;
}
