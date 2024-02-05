#define VD_INTERNAL_SOURCE_FILE 1
#include "strmap.h"
#include "hash.h"

typedef struct BinPrefix BinPrefix;
struct BinPrefix {
    BinPrefix *next;
    u32        key_len;
    bool       used;
    char      *key_rest;
    char       key_prefix[43];
};

bool vd__strmap_set(VD_StrMapHeader *map, u32 tsize, VD_str key, void *value)
{
    const u32 bin_size = sizeof(BinPrefix) + tsize;
    if (map == 0) {
        // Initialize the map
        map = (VD_StrMapHeader*)vd_realloc(map->allocator, 0, 0, sizeof(VD_StrMapHeader) + bin_size * map->cap);
    }

    size_t hash = VD_HASH_STR(key, VD_HASH_DEFAULT_SEED);

    return false;
}
