#ifndef VD_HASH_H
#define VD_HASH_H

#include "common.h"

VD_INLINE u64 vd_hash(const void *data, u64 len, u32 seed)
{
    const u64 m = 0x5bd1e995;
    const u64 r = 24;

    u64 h = seed ^ len;

    const u8 *bytes = (const u8*)data;

    while (len >= 4)
    {
        u32 k = *(u32*)bytes;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        bytes += 4;
        len -= 4;   
    }

    switch (len) {
        case 3: h ^= bytes[2] << 16;
        case 2: h ^= bytes[1] << 8;
        case 1: h ^= bytes[0];
                h *= m;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#define VD_HASH_STR(in, seed) vd_hash(in.data, in.len, seed)

#define VD_HASH_DEFAULT_SEED (0x9747b28c)

#endif