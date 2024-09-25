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
#define VD_STRMAP_DEINIT(m)          vd_realloc(VD_STRMAP_HEADER(m)->allocator, (umm)VD_STRMAP_HEADER(m), VD_STRMAP_HEADER(m)->cap_total, 0)


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
#define strmap              VD_STRMAP
/**
 * @brief Initialize a strmap.
 * @param m The map. Must be a pointer (e.g int *map).
 * @param a The allocator.
 */
#define strmap_init(m,a)    VD_STRMAP_INIT(m,a)

/**
 * @brief Set a value in the map.
 * @param m The map.
 * @param k The key.
 * @param v The value. Must be a pointer (e.g &value).
 */
#define strmap_set(m,k,v)   VD_STRMAP_SET(m,k,v)

/**
 * @brief Get a value in the map.
 * @param m The map.
 * @param k The key.
 * @param v The value. Must be a pointer (e.g &value).
 */
#define strmap_get(m,k,v)   VD_STRMAP_GET(m,k,v)

/**
 * @brief Delete a specific key from the map.
 * @param m The map.
 * @param k The key.
 */
#define strmap_del(m,k)     VD_STRMAP_DEL(m,k)

/**
 * @brief Deinitialize the map.
 * @param m The map.
 */
#define strmap_deinit(m)    VD_STRMAP_DEINIT(m)

#endif // VD_ABBREVIATIONS
#endif // !VD_STRMAP_H