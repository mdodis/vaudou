#define VD_INTERNAL_SOURCE_FILE 1
#include "mm.h"

#include "arena.h"
#include "instance.h"
#include "builtin.h"
#include "vd_atomic.h"

#include "flecs.h"
#define VD_BUDDY_ALLOC_IMPLEMENTATION
#include "vd_buddy_alloc.h"


typedef struct VD_EntityAllocationInfo VD_EntityAllocationInfo;

struct VD_EntityAllocationInfo {
    /** The size of the allocation (including this header). */
    size_t 					size;

    /** The next allocation for this entity. */
    VD_EntityAllocationInfo *next;
};

struct VD_MM {

    struct {
        Arena        arena;
        VD_Allocator allocator;
    } global;

    struct {
        Arena 			arena;
        VD_Allocator 	allocator;
    } frame;

    struct {
        VD_BuddyAlloc 			allocator;
        VD_EntityAllocationInfo free_list;
    } entity;
};

/**
 * @brief Adds an allocation to @param root.
 * @param root The root of the list.
 * @param info The allocation to add.
 * @return void
 * @note This function does not check if the allocation is already in the list.
 * @note This function makes no assumptions if the allocation and root belong in the same entity.
 */
static void add_entity_allocation(VD_EntityAllocationInfo *root, VD_EntityAllocationInfo *info)
{
    VD_EntityAllocationInfo *prev_next;
    do
    {
        prev_next = (VD_EntityAllocationInfo*)load_ptr(&root->next);
        info->next = prev_next;

        // @note: not sure how this can happen...
        if (prev_next == info) {
            break;
        }

    } while (compare_and_swap_ptr(&root->next, info, prev_next));
}

static VD_BUDDY_ALLOCATION_PROC(buddy_alloc_proc)
{
    VD_Allocator *allocator = (VD_Allocator *)usrdata;
    return (void*)allocator->proc_alloc((umm)ptr, prevsize, newsize, usrdata);
}

static VD_PROC_ALLOC(vd_entity_alloc)
{
    ecs_entity_t entity = (ecs_entity_t)c;
    ecs_world_t *world = vd_instance_get_world(vd_instance_get());
    VD_MM *mm = vd_instance_get_mm(vd_instance_get());

    if (newsize == 0)
    {
        return 0;
    }

    VD_EntityAllocationInfo *info = vd_buddy_alloc_realloc(
        &mm->entity.allocator,
        0,
        sizeof(VD_EntityAllocationInfo) + newsize);

    info->next = 0;
    info->size = sizeof(VD_EntityAllocationInfo) + newsize;

    const MemoryComponent *memory;
    if (ecs_has(world, entity, MemoryComponent)) {
        memory = ecs_get(world, entity, MemoryComponent);
        add_entity_allocation((VD_EntityAllocationInfo*)memory->opaque_info_ptr, info);
    } else {
        ecs_set(world, entity, MemoryComponent,{
            .opaque_info_ptr = info,
        });
    }

    if (prevsize != 0) {
        memcpy((char*)info + sizeof(VD_EntityAllocationInfo), (void*)ptr, prevsize);
    }

    return (umm)((char *)info + sizeof(VD_EntityAllocationInfo));
}

VD_MM *vd_mm_create()
{
    return calloc(1, sizeof(VD_MM));
}

void vd_mm_init(VD_MM *mm)
{

    mm->global.arena = arena_new(VD_MEGABYTES(4), vd_memory_get_system_allocator());
    mm->global.allocator = (VD_Allocator) { .c = &mm->global.arena, .proc_alloc = vd_arena_proc_alloc };

    mm->frame.arena = arena_new(VD_MEGABYTES(4), vd_memory_get_system_allocator());
    mm->frame.allocator = (VD_Allocator) { .c = &mm->frame.arena, .proc_alloc = vd_arena_proc_alloc };

    mm->entity.free_list.next = 0;
    mm->entity.free_list.size = 0;
    vd_buddy_alloc_init(
        &mm->entity.allocator,
        buddy_alloc_proc,
        vd_memory_get_system_allocator(),
        VD_MEGABYTES(4),
        8);
}

void *vd_mm_alloc(VD_MM *mm, VD_AllocationInfo *info)
{
    switch (info->tag)
    {
        case VD_MM_GLOBAL:
        {
            return arena_alloc_t(&mm->global.arena, info->size);
        } break;

        case VD_MM_FRAME:
        {
            return arena_alloc_t(&mm->frame.arena, info->size);
        } break;

        case VD_MM_ENTITY:
        {
            return (void*)vd_entity_alloc(0, 0, info->size, (void*)info->entity_id);
        } break;

        default:
        {
            return 0;
        } break;
    }

    return 0;
}

VD_Allocator *vd_mm_get_global_allocator(VD_MM *mm)
{
    return &mm->global.allocator;
}

VD_Arena *vd_mm_get_frame_arena(VD_MM *mm)
{
    return &mm->frame.arena;
}

VD_Allocator *vd_mm_get_frame_allocator(VD_MM *mm)
{
    return &mm->frame.allocator;
}

VD_Allocator vd_mm_make_entity_allocator(VD_MM *mm, u64 entity_id)
{
    VD_Allocator allocator = { 0 };
    static_assert(sizeof(void*) >= sizeof(u64), "Pointer must be able to hold a u64.");

    allocator.proc_alloc = vd_entity_alloc;
    allocator.c = (void*)entity_id;

    return allocator;
}

void vd_mm_end_frame(VD_MM *mm)
{
    arena_reset(&mm->frame.arena);
}

void vd_mm_get_stats(VD_MM *mm, VD_MM_Stats *stats)
{
    vd_arena_get_stats(&mm->frame.arena, &stats->frame_used, &stats->frame_total);

    vd_buddy_alloc_get_stats(
        &mm->entity.allocator,
        &stats->entity_used,
        &stats->entity_total,
        &stats->num_entity_blocks,
        &stats->num_free_entity_blocks);
}

void vd_mm_deinit(VD_MM *mm)
{
    VD_MM_Stats stats;
    vd_mm_get_stats(mm, &stats);
    VD_LOG_FMT(
        "Memory",
        "Frame Memory: %{u64}/%{u64} bytes",
        stats.frame_used, stats.frame_total);

    VD_LOG_FMT(
        "Memory",
        "Entity Memory: %{u64}/%{u64} bytes, %{u64} free blocks %{u64} used blocks",
        stats.entity_used, stats.entity_total,
        stats.num_free_entity_blocks, stats.num_entity_blocks);
}

VD_DTOR_PROC(MemoryComponentDtor)
{
    MemoryComponent *memory = (MemoryComponent*)ptr;
    VD_MM *mm = vd_instance_get_mm(vd_instance_get());

    for (int i = 0; i < count; ++i) {
        VD_EntityAllocationInfo *info = memory->opaque_info_ptr;
        add_entity_allocation(&mm->entity.free_list, info);
    }
}

void GarbageCollectTask(ecs_iter_t *t)
{
    VD_MM *mm = vd_instance_get_mm(vd_instance_get());
    VD_EntityAllocationInfo *curr_info = mm->entity.free_list.next;
    while (curr_info != 0) {
        vd_buddy_alloc_realloc(&mm->entity.allocator, curr_info, 0);
        curr_info = curr_info->next;
    }

    mm->entity.free_list.next = 0;
}

void FreeFrameAllocationSystem(ecs_iter_t *t)
{
    VD_MM *mm = vd_instance_get_mm(vd_instance_get());
    vd_mm_end_frame(mm);
}