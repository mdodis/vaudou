#define VD_INTERNAL_SOURCE_FILE 1
#include "strmap.h"
#include "hash.h"

VD_bool vd__strmap_init(VD_StrMapHeader *map, VD_Allocator *allocator, u32 tsize, u32 cap)
{
    const u32 bin_size = sizeof(VD_BinPrefix) + tsize;
    map = (VD_StrMapHeader*)vd_realloc(
        map->allocator, 
        0, 
        0, 
        sizeof(VD_StrMapHeader) + bin_size * map->cap);

    map->cap_total = cap;
    map->cap = (u32)(((float)(cap)) * 0.86f);
    map->tsize = tsize;
    map->allocator = allocator;
    return true;
}

VD_BinPrefix *vd__strmap_get_bin(void *map, VD_str key)
{
    // Compute header
    VD_StrMapHeader *header = VD_STRMAP_HEADER(map);
    size_t hash = VD_HASH_STR(key, VD_HASH_DEFAULT_SEED);
    size_t bin_index = hash % header->cap;
}

bool vd__strmap_set(VD_StrMapHeader *map, u32 tsize, VD_str key, void *value)
{
    const u32 bin_size = sizeof(VD_BinPrefix) + tsize;
    size_t hash = VD_HASH_STR(key, VD_HASH_DEFAULT_SEED);
    size_t bin_index = hash % map->cap;

    return false;
}
