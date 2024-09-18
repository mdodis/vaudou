#ifndef VD_ARRAY_H
#define VD_ARRAY_H
#include "common.h"
#include "allocator.h"
#include <string.h>

typedef struct {
    size_t        len;
    size_t        cap;
    VD_Allocator *allocator;
} VD_ArrayHeader;

#define VD_ARRAY_HEADER(a)          ((VD_ArrayHeader*)((u8*)(a) - sizeof(VD_ArrayHeader)))
#define VD_ARRAY_INIT(a, allocator) ((a) = vd_array_grow(a, sizeof(*(a)), 1, 0, allocator))
#define VD_ARRAY_DEINIT(a)          (vd_free(VD_ARRAY_ALC(a), VD_ARRAY_HEADER(a), VD_ARRAY_CAP(a) * sizeof(*a)))
#define VD_ARRAY_ADD(a, v)          (VD_ARRAY_CHECK_GROW(a,1), (a)[VD_ARRAY_HEADER(a)->len++] = (v))
#define VD_ARRAY_PUSH(a)            (VD_ARRAY_CHECK_GROW(a,1), VD_ARRAY_HEADER(a)->len++)
#define VD_ARRAY_LEN(a)             ((a) ? VD_ARRAY_HEADER(a)->len : 0)
#define VD_ARRAY_CAP(a)             ((a) ? VD_ARRAY_HEADER(a)->cap : 0)
#define VD_ARRAY_ALC(a)             ((a) ? VD_ARRAY_HEADER(a)->allocator : 0)
#define VD_ARRAY_GROW(a, b, c)      ((a) = vd_array_grow((a), sizeof(*(a)), (b), (c), VD_ARRAY_ALC(a)))
#define VD_ARRAY_POP(a)             (VD_ARRAY_HEADER(a)->len--, (a)[VD_ARRAY_HEADER(a)->len])
#define VD_ARRAY_LAST(a)            ((a)[VD_ARRAY_HEADER(a)->len - 1])
#define VD_ARRAY_DEL(a, i)          VD_ARRAY_DELN(a, i, 1)
#define VD_ARRAY_DELSWAP(a, i)      ((a)[i] = VD_ARRAY_LAST(a), VD_ARRAY_HEADER(a)->len -= 1)
#define VD_ARRAY_DELN(a, i, n)      \
    (memmove(&(a)[i], &(a)[i + n], sizeof(*(a)) * (VD_ARRAY_HEADER(a)->len - (n) - (i))), \
    VD_ARRAY_HEADER(a)->len -= (n))
#define VD_ARRAY_CHECK_GROW(a, n)   \
    ((!(a) || VD_ARRAY_HEADER(a)->len + (n) > VD_ARRAY_HEADER(a)->cap) \
    ? (VD_ARRAY_GROW(a, n, 0), 0) : 0)

VD_INLINE void *vd_array_grow(
    void *a, 
    size_t tsize, 
    size_t addlen, 
    size_t mincap, 
    VD_Allocator* allocator)
{
    size_t min_len = VD_ARRAY_LEN(a) + addlen;

    if (min_len > mincap) {
        mincap = min_len;
    }

    if (mincap <= VD_ARRAY_CAP(a)) {
        return a;
    }

    if (mincap < (2 * VD_ARRAY_CAP(a))) {
        mincap = 2 * VD_ARRAY_CAP(a);
    } else if (mincap < 4) {
        mincap = 4;
    }

    void *b = (void*)vd_realloc(
        allocator, 
        (umm)(a ? VD_ARRAY_HEADER(a) : 0),
        VD_ARRAY_CAP(a) == 0 ? 0 : tsize * VD_ARRAY_CAP(a) + sizeof(VD_ArrayHeader),
        tsize * mincap + sizeof(VD_ArrayHeader));
    
    b = (char*)b + sizeof(VD_ArrayHeader);
    if (a == 0) {
        VD_ARRAY_HEADER(b)->len = 0;
        VD_ARRAY_HEADER(b)->allocator = allocator;
    }

    VD_ARRAY_HEADER(b)->cap = mincap;
    return b;
}

#define VD_ARRAY

#if VD_ABBREVIATIONS
#define array_init      VD_ARRAY_INIT
#define array_deinit    VD_ARRAY_DEINIT
#define array_add       VD_ARRAY_ADD
#define array_len       VD_ARRAY_LEN
#define array_cap       VD_ARRAY_CAP
#define array_pop       VD_ARRAY_POP
#define array_last      VD_ARRAY_LAST
#define array_delswap   VD_ARRAY_DELSWAP
#define array_del       VD_ARRAY_DEL
#define array_deln      VD_ARRAY_DELN
#define dynarray        VD_ARRAY
#endif

#endif