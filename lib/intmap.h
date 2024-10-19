#ifndef VD_INTMAP_H
#define VD_INTMAP_H
#include "vd_common.h"

typedef struct VD_IntMapEntry VD_IntMapEntry;

struct VD_IntMapEntry {
    VD_IntMapEntry      *next;
    int                 used;
    u64                 k;
    u64                 v;
};

typedef enum {
    VD_INTMAP_FLAG_NO_AUTOGROW = 1 << 0,
} VD_IntMapFlags;

typedef struct {
    VD_IntMapEntry  *table;
    u64             cap;
    u64             cap_total;
    u64             usecount;
    VD_Allocator    allocator;
    VD_IntMapFlags  flags;
} VD_IntMap;

void vd_intmap_init(VD_IntMap *map, VD_Allocator *allocator, u64 cap, VD_IntMapFlags flags);
int vd_intmap_tryget(VD_IntMap *map, u64 k, u64 *v);
int vd_intmap_set(VD_IntMap *map, u64 k, u64 v);
int vd_intmap_check_grow(VD_IntMap *map);
void vd_intmap_del(VD_IntMap *map, u64 k);
void vd_intmap_deinit(VD_IntMap *map);


#endif // ! VD_INTMAP_H
