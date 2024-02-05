#ifndef VD_STRMAP_H
#define VD_STRMAP_H

#include "common.h"
#include "allocator.h"
#include "str.h"

typedef struct {
    u32           cap;
    u32           cap_total;
    u32           tsize;
    VD_Allocator *allocator;
} VD_StrMapHeader;

typedef struct VD_BinPrefix VD_BinPrefix;
struct VD_BinPrefix {
    VD_BinPrefix *next;
    u32        key_len;
    bool       used;
    char      *key_rest;
    char       key_prefix[43];
};

#define VD_STRMAP_HEADER(m)       ((VD_StrMapHeader*)((u8*)(m) - sizeof(VD_StrMapHeader)))
#define VD_STRMAP_INIT(m, allocator)
#define VD_STRMAP_SET(m, k, v) \
    vd__strmap_set(VD_STRMAP_HEADER(m), sizeof(v), k, &v)

VD_bool vd__strmap_init(VD_StrMapHeader *map, VD_Allocator *allocator, u32 tsize, u32 cap);
VD_bool vd__strmap_set(VD_StrMapHeader *map, u32 tsize, VD_str key, void *value);
VD_BinPrefix *vd__strmap_get_bin(void *map, VD_str key);

/**
 * @brief Use this to indicate that the following pointer is a strmap.
 */
#define VD_STRMAP 

#if VD_ABBREVIATIONS
#define strmap VD_STRMAP
#endif

#endif