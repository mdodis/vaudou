#include "handlemap.h"

#include "hash.h"
#include "intmap.h"

#include <assert.h>
#include <string.h>

typedef struct VD_HandleMap {
    umm             arr;
    size_t          arr_len;
    size_t          arr_cap;
    VD_IntMap       idmap;
    size_t          elsize;
    u64             next_id;
    VD_Allocator    allocator;
    void            (*on_free_object)(void *object);
} VD_HandleMap;

typedef struct {
    int refcount;
    int refmode;
    u64 id;
} EntryMetadata;

static void *get_slot_value_ptr(VD_HandleMap *map, u64 slot)
{
    return (void*)(map->arr + (sizeof(EntryMetadata) + map->elsize) * slot + sizeof(EntryMetadata));
}

static EntryMetadata *get_slot_metadata_ptr(VD_HandleMap *map, u64 slot)
{
    return (EntryMetadata*)(map->arr + (sizeof(EntryMetadata) + map->elsize) * slot);
}

static void free_object_null(void *object) {}

void *vd_handlemap__init(size_t elsize, VD_HandleMapInitInfo *info)
{
    VD_HandleMap *hdr = (VD_HandleMap*)info->allocator->proc_alloc(
        0,
        0,
        sizeof(VD_HandleMap),
        info->allocator->c);

    hdr->on_free_object = info->on_free_object;
    if (hdr->on_free_object == 0) {
        hdr->on_free_object = free_object_null;
    }

    hdr->elsize     = elsize;
    hdr->allocator  = *info->allocator;
    hdr->arr_cap    = info->initial_capacity;
    hdr->arr_len    = 0;
    hdr->next_id    = 1;

    vd_intmap_init(&hdr->idmap, &hdr->allocator, info->initial_capacity, 0);

    size_t array_element_size = sizeof(EntryMetadata) + hdr->elsize;

    hdr->arr = hdr->allocator.proc_alloc(0, 0, array_element_size * hdr->arr_cap, hdr->allocator.c);
    memset((void*)hdr->arr, 0, array_element_size * hdr->arr_cap);

    return (void*)hdr;
}

static u64 allocate_slot(VD_HandleMap *hdr)
{
    size_t array_element_size = sizeof(EntryMetadata) + hdr->elsize;
    if (hdr->arr_len == hdr->arr_cap) {
        hdr->arr = hdr->allocator.proc_alloc(
            hdr->arr,
            array_element_size * hdr->arr_cap,
            array_element_size * hdr->arr_cap * 2,
            hdr->allocator.c);
        hdr->arr_cap *= 2;
    }

    return hdr->arr_len++;
}

VD_Handle vd_handlemap__register(VD_HandleMap *hdr, void *valueptr, VD_HandleMapRegisterInfo *info)
{
    u64 id = hdr->next_id++;

    u64 slot = allocate_slot(hdr);

    vd_intmap_set(&hdr->idmap, id, slot);

    EntryMetadata *metadata_ptr = get_slot_metadata_ptr(hdr, slot);
    void *slot_value_ptr = get_slot_value_ptr(hdr, slot);

    metadata_ptr->id = id;
    metadata_ptr->refcount = 1;
    metadata_ptr->refmode = info->ref_mode;

    memcpy((char*)slot_value_ptr, valueptr, hdr->elsize);

    return (VD_Handle) {
        .id = id,
        .map = (void*)hdr,
    };
}

void *vd_handle_use(VD_Handle *handle, VD_HandleMapUseMode mode)
{
    VD_HandleMap *map = handle->map;

    u64 slot;
    if (!vd_intmap_tryget(&map->idmap, handle->id, &slot)) {
        return 0;
    }

    return get_slot_value_ptr(map, slot);
}

VD_Handle vd_handlemap_copy(VD_Handle *handle)
{
    VD_HandleMap *map = handle->map;
    u64 slot;
    if (!vd_intmap_tryget(&map->idmap, handle->id, &slot)) {
        return (VD_Handle) {};
    }

    EntryMetadata *metadata_ptr = get_slot_metadata_ptr(map, slot);
    if (metadata_ptr->refmode == VD_HANDLEMAP_REF_MODE_COUNT) {
        metadata_ptr->refcount++;
    }

    return *handle;
}

void vd_handle_drop(VD_Handle *handle)
{
    VD_HandleMap *map = handle->map;
    u64 slot;
    if (!vd_intmap_tryget(&map->idmap, handle->id, &slot)) {
        return;
    }

    EntryMetadata *metadata_ptr = get_slot_metadata_ptr(map, slot);
    if (metadata_ptr->refmode == VD_HANDLEMAP_REF_MODE_COUNT) {
        metadata_ptr->refcount--;

        assert(!(metadata_ptr->refcount < 0));
        if (metadata_ptr->refcount == 0) {
            void *value_ptr = get_slot_value_ptr(map, slot);
            map->on_free_object(value_ptr);
        }
    }
    handle->id = 0;
}
