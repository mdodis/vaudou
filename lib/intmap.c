#include "intmap.h"
#include "hash.h"

#include <string.h>

void vd_intmap_init(VD_IntMap *map, VD_Allocator *allocator, u64 cap, VD_IntMapFlags flags)
{
    map->allocator = *allocator;
    map->flags = flags;
    map->cap_total = cap;
    map->usecount = 0;
    map->cap = map->cap_total - (u64)((float)map->cap_total * 0.20f);
    size_t byte_size = map->cap_total * sizeof(VD_IntMapEntry);
    map->table = (VD_IntMapEntry*)map->allocator.proc_alloc(
        0,
        0,
        byte_size,
        map->allocator.c);
    memset(map->table, 0, byte_size);
}

int vd_intmap_tryget(VD_IntMap *map, u64 k, u64 *v)
{
    u64 hash = vd_hash(&k, sizeof(k), 0xFACE0FF);
    u64 index = hash % map->cap;

    VD_IntMapEntry *entry = &map->table[index];

    while (entry->used && entry->k != k) {
        if (entry->next) {
            entry = entry->next;
        } else {
            return 0;
        }
    }

    *v = entry->v;
    return 1;
}

int vd_intmap_set(VD_IntMap *map, u64 k, u64 v)
{
    if (!(map->flags & VD_INTMAP_FLAG_NO_AUTOGROW)) vd_intmap_check_grow(map);

    u64 hash = vd_hash(&k, sizeof(k), 0xFACE0FF);
    u64 index = hash % map->cap;

    VD_IntMapEntry *existing = &map->table[index];
    VD_IntMapEntry *curr = existing;

    if (existing->used) {
        u64 cursor = map->cap_total;

        while (curr->used && cursor > 0) {
            cursor--;
            curr = &map->table[cursor];
        }

        if (curr->used) {
            return 0;
        }

        VD_IntMapEntry *chain = existing;

        while (chain->next) {
            chain = chain->next;
        }

        chain->next = curr;
    }

    curr->used = 1;
    curr->k = k;
    curr->v = v;
    curr->next = 0;
    map->usecount++;

    return 1;
}

int vd_intmap_check_grow(VD_IntMap *map)
{
    if (map->usecount != map->cap_total)
    {
        return 0;
    }

    VD_IntMap new_map;
    vd_intmap_init(&new_map, &map->allocator, map->cap_total * 2, map->flags);

    for (u64 i = 0; i < map->cap_total; ++i) {
        VD_IntMapEntry *entry = &map->table[i];

        if (!entry->used) {
            continue;
        }

        vd_intmap_set(&new_map, entry->k, entry->v);
    }

    vd_intmap_deinit(map);

    *map = new_map;
    return 1;
}

void vd_intmap_del(VD_IntMap *map, u64 k)
{
    u64 hash = vd_hash(&k, sizeof(k), 0xFACE0FF);
    u64 index = hash % map->cap;

    VD_IntMapEntry *entry = &map->table[index];

    while (entry->used && entry->k != k) {
        if (entry->next) {
            entry = entry->next;
        } else {
            return;
        }
    }

    entry->used = 0;
    entry->k = 0;
    entry->v = 0;
    if (map->usecount > 0) {
        map->usecount--;
    }
}

void vd_intmap_deinit(VD_IntMap *map)
{
    map->allocator.proc_alloc(
        (umm)map->table,
        map->cap_total * sizeof(*map->table),
        0,
        map->allocator.c);
}
