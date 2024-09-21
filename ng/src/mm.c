#define VD_INTERNAL_SOURCE_FILE 1
#include "mm.h"
#include "flecs.h"

struct VD_MM {

	struct {
		VD_AllocationInfo *free_list;
	} e;
};

void vd_mm_init(VD_MM *mm);
void *vd_mm_alloc(VD_MM *mm, VD_AllocationInfo *info);
void vd_mm_deinit(VD_MM *mm);

void GarbageCollectTask(ecs_iter_t *t)
{

}