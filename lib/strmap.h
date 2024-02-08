// strmap.h
// Author: Michael Dodis
// 
// A map of string slices to values. The map is implemented as a hash table.
#ifndef VD_STRMAP_H
#define VD_STRMAP_H
#include "common.h"
#include "allocator.h"
#include "str.h"
#include <string.h>

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

typedef enum {
    /** 
     * ON:  Create a new bin if it does not exist.
     * OFF: Will return NULL if the bin does not exist.
     */
    VD__GET_BIN_FLAGS_CREATE = 1 << 1,
    VD__GET_BIN_FLAGS_SET_UNUSED = 1 << 2,
} VD__GetBinFlags;

#define VD_STRMAP_DEFAULT_CAP        2048
#define VD_STRMAP_HEADER(m)          ((VD_StrMapHeader*)((u8*)(m) - sizeof(VD_StrMapHeader)))
#define VD_STRMAP_INIT(m, allocator) \
    m = vd__strmap_init(allocator, sizeof(*m), VD_STRMAP_DEFAULT_CAP)
#define VD_STRMAP_SET(m,k,v)         vd__strmap_set(m, k, (void*)v)
#define VD_STRMAP_GET(m,k,v)         vd__strmap_get(m, k, (void*)v)
#define VD_STRMAP_DEL(m,k)           (vd__strmap_get_bin(m, k, VD__GET_BIN_FLAGS_SET_UNUSED) != 0)

void *vd__strmap_init(VD_Allocator *allocator, u32 tsize, u32 cap);
VD_BinPrefix *vd__strmap_get_bin(void *map, VD_str key, VD__GetBinFlags op);

VD_INLINE VD_bool vd__strmap_set(void *map, VD_str key, void *value)
{
    VD_BinPrefix *bin = vd__strmap_get_bin(map, key, VD__GET_BIN_FLAGS_CREATE);
    if (!bin) {
        return false;
    }

    u8 *bin_data = (((u8*)bin) + sizeof(VD_BinPrefix));
    u32 tsize = VD_STRMAP_HEADER(map)->tsize;
    memcpy(bin_data, value, tsize);

    return true;
}

VD_INLINE VD_bool vd__strmap_get(void *map, VD_str key, void *value)
{
    VD_BinPrefix *bin = vd__strmap_get_bin(map, key, 0);
    if (!bin) {
        return false;
    }

    u8 *bin_data = (((u8*)bin) + sizeof(VD_BinPrefix));
    memcpy(value, bin_data, VD_STRMAP_HEADER(map)->tsize);

    return true;
}

/**
 * @brief Use this to indicate that the following pointer is a strmap.
 */
#define VD_STRMAP 

#if VD_ABBREVIATIONS
#define strmap      VD_STRMAP
#define strmap_init VD_STRMAP_INIT
#define strmap_set  VD_STRMAP_SET
#define strmap_get  VD_STRMAP_GET
#define strmap_del  VD_STRMAP_DEL
#endif

#endif