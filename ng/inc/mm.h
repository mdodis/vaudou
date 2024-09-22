#ifndef VD_MM_H
#define VD_MM_H
#include "common.h"
#include "allocator.h"
#include "arena.h"

/**
 * @brief Memory management tags.
 * Designates the suggested lifetime of memory.
 */
typedef enum {
    /**
     * @brief Memory persists for the lifetime of the application.
     */
    VD_MM_GLOBAL,

    /**
     * @brief Memory persists for the lifetime of the current frame.
     */
    VD_MM_FRAME,

    /**
     *  @brief Memory persists for the lifetime of the current entity.
     */
    VD_MM_ENTITY,
} VD_MM_Tag;

typedef struct VD_MM VD_MM;
typedef struct VD_AllocationInfo VD_AllocationInfo;

struct VD_AllocationInfo {
    /* Input */
    void        *ptr;
    size_t       size;
    VD_MM_Tag    tag;

    /* Reserved */
    int                  used;
    VD_AllocationInfo   *free_next;
    VD_AllocationInfo   *free_prev;
};

#define VD_MM_ALLOC(n, t) vd_mm_alloc(vd_instance_get_mm(vd_instance_get()),  \
    &(VD_AllocationInfo) {                                                    \
        .ptr = 0,                                                             \
        .size = n,                                                            \
        .tag = t,                                                             \
    });

#define VD_MM_ALLOC_STRUCT(s, t) VD_MM_ALLOC(sizeof(s), t)
#define VD_MM_ALLOC_ARRAY(s, n, t) VD_MM_ALLOC(sizeof(s)*n, t)

#define VD_MM_FRAME_ALLOCATOR() vd_mm_get_frame_allocator(vd_instance_get_mm(vd_instance_get()))
#define VD_MM_FRAME_ARENA() vd_mm_get_frame_arena(vd_instance_get_mm(vd_instance_get()))
#define VD_MM_FRAME_ALLOC(s) VD_MM_ALLOC(s, VD_MM_FRAME)
#define VD_MM_FRAME_ALLOC_STRUCT(s) (s*)VD_MM_ALLOC(sizeof(s), VD_MM_FRAME)
#define VD_MM_FRAME_ALLOC_ARRAY(s, n) (s*)VD_MM_FRAME_ALLOC(sizeof(s)*n)

VD_MM *vd_mm_create();
void vd_mm_init(VD_MM *mm);
void *vd_mm_alloc(VD_MM *mm, VD_AllocationInfo *info);

VD_Arena *vd_mm_get_frame_arena(VD_MM *mm);
VD_Allocator *vd_mm_get_frame_allocator(VD_MM *mm);
void vd_mm_end_frame(VD_MM *mm);

void vd_mm_deinit(VD_MM *mm);
extern void GarbageCollectTask(struct ecs_iter_t *t);

#endif