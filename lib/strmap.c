#define VD_INTERNAL_SOURCE_FILE 1
#include "strmap.h"
#include "hash.h"
#include <string.h>

#define GET_BIN_AT(m, i) \
    ((VD_BinPrefix*)(((u8*)m) + (VD_STRMAP_HEADER(m)->tsize + sizeof(VD_BinPrefix)) * i))

static bool check_key(VD_str check, VD_BinPrefix* against)
{
    u32 prefix_len = sizeof(against->key_prefix);
    u32 first_check_len = prefix_len < check.len ? prefix_len : check.len;
    u32 second_check_len = check.len - prefix_len;

    if (against->key_len != check.len) {
        return false;
    }

    if (memcmp(against->key_prefix, check.data, first_check_len)) {
        return false;
    }

    return check.len < prefix_len 
        ? true
        : (memcmp(against->key_rest, check.data + prefix_len, second_check_len) == 0);
}

static void copy_key(str key, VD_BinPrefix *bin)
{
    u32 prefix_len = sizeof(bin->key_prefix);
    u32 key_len = key.len;
    bin->key_len = key_len;

    u32 first_copy_len = prefix_len < key_len ? prefix_len : key_len;

    memcpy(bin->key_prefix, key.data, first_copy_len);

    if (key_len > prefix_len) {
        bin->key_rest = (char*)vd_realloc(
            VD_STRMAP_HEADER(bin)->allocator, 
            0, 
            0, 
            key_len - prefix_len);
        memcpy(bin->key_rest, key.data + prefix_len, key_len - prefix_len);
    }
    
}

void *vd__strmap_init(VD_Allocator *allocator, u32 tsize, u32 cap)
{
    VD_StrMapHeader *map;
    const u32 bin_size = sizeof(VD_BinPrefix) + tsize;
    map = (VD_StrMapHeader*)vd_realloc(
        allocator, 
        0, 
        0, 
        sizeof(VD_StrMapHeader) + bin_size * cap);

    memset(map, 0, sizeof(VD_StrMapHeader) + bin_size * cap);

    map->cap_total = cap;
    map->cap = (u32)(((float)(cap)) * 0.86f);
    map->tsize = tsize;
    map->allocator = allocator;
    return (void*)((u8*)map + sizeof(VD_StrMapHeader));
}

VD_BinPrefix *vd__strmap_get_bin(void *map, VD_str key, VD__GetBinFlags op)
{
    size_t hash = VD_HASH_STR(key, VD_HASH_DEFAULT_SEED);
    size_t bin_index = hash % VD_STRMAP_HEADER(map)->cap;

    VD_BinPrefix* existing_bin = GET_BIN_AT(map, bin_index);

    if (existing_bin->used) {
        bool found = false;

        if (check_key(key, existing_bin)) {
            found = true;
        }

        while (!found && (existing_bin->next != 0)) {
            existing_bin = existing_bin->next;

            if (check_key(key, existing_bin)) {
                found = true;
            }
        }
        
        if (found) {
            if (op & VD__GET_BIN_FLAGS_SET_UNUSED) {
                existing_bin->used = false;

                if (existing_bin->key_rest) {
                    vd_free(
                        VD_STRMAP_HEADER(map)->allocator, 
                        (umm)existing_bin->key_rest, 
                        sizeof(existing_bin->key_prefix) - existing_bin->key_len);
                }
            }

            return existing_bin;
        }
    }

    if (!(op & VD__GET_BIN_FLAGS_CREATE)) {
        return 0;
    }

    if (!existing_bin->used) {
        copy_key(key, existing_bin);
        existing_bin->used = true;
        existing_bin->next = 0;
        return existing_bin;
    }

    u32 cursor = VD_STRMAP_HEADER(map)->cap_total;
    while ((cursor > 0) && (GET_BIN_AT(map, cursor)->used)) {
        cursor--;
    }

    if (cursor == 0) {
        return 0;
    }

    VD_BinPrefix* new_bin = GET_BIN_AT(map, cursor);
    new_bin->used = true;
    new_bin->next = 0;
    copy_key(key, new_bin);

    return new_bin;
}