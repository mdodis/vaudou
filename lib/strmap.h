#ifndef VD_STRMAP_H
#define VD_STRMAP_H

#include "common.h"
#include "allocator.h"
#include "str.h"

typedef struct {
    u32           cap;
    u32           tsize;
    VD_Allocator *allocator;
} VD_StrMapHeader;

#define VD_STRMAP_HEADER(m)       ((VD_StrMapHeader*)((u8*)(m) - sizeof(VD_StrMapHeader)))
#define VD_STRMAP_SET(m, k, v) \
    vd__strmap_set(VD_STRMAP_HEADER(m), sizeof(v), k, &v)

VD_bool vd__strmap_set(VD_StrMapHeader *map, u32 tsize, VD_str key, void *value);

/**
 * @brief Use this to indicate that the following pointer is a strmap.
 */
#define VD_STRMAP 

#if VD_ABBREVIATIONS
#define strmap VD_STRMAP
#endif

#endif