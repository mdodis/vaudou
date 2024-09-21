#ifndef VD_MM_H
#define VD_MM_H
#include "common.h"
#include "allocator.h"

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
     * @brief Memory persists for the lifetime of the current function (single threaded).
     */
    VD_MM_FUNCTION,

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

void vd_mm_init(VD_MM *mm);
void *vd_mm_alloc(VD_MM *mm, VD_AllocationInfo *info);
void vd_mm_deinit(VD_MM *mm);
extern void GarbageCollectTask(struct ecs_iter_t *t);

#endif