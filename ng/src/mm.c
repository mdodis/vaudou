#define VD_INTERNAL_SOURCE_FILE 1
#include "mm.h"
#include "flecs.h"
#include "arena.h"

struct VD_MM {

	struct {
		Arena 			arena;
		VD_Allocator 	allocator;
	} frame;

	struct {
		VD_AllocationInfo *free_list;
	} entity;
};

VD_MM *vd_mm_create()
{
	return calloc(1, sizeof(VD_MM));
}

void vd_mm_init(VD_MM *mm)
{
	mm->frame.arena = arena_new(VD_MEGABYTES(4), vd_memory_get_system_allocator());
	mm->frame.allocator = (VD_Allocator) { .c = &mm->frame.arena, .proc_alloc = vd_arena_proc_alloc };
}

void *vd_mm_alloc(VD_MM *mm, VD_AllocationInfo *info)
{
	switch (info->tag)
	{
		case VD_MM_FRAME:
		{
			return arena_alloc_t(&mm->frame.arena, info->size);
		} break;

		default:
		{
			return 0;
		} break;
	}

	return 0;
}

VD_Arena *vd_mm_get_frame_arena(VD_MM *mm)
{
	return &mm->frame.arena;
}

VD_Allocator *vd_mm_get_frame_allocator(VD_MM *mm)
{
	return &mm->frame.allocator;
}


void vd_mm_end_frame(VD_MM *mm)
{
	arena_reset(&mm->frame.arena);
}

void vd_mm_deinit(VD_MM *mm);

void GarbageCollectTask(ecs_iter_t *t)
{

}