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
    VD_MM_GLOBAL = 1,

    /**
     * @brief Memory persists for the lifetime of the current frame.
     */
    VD_MM_FRAME = 2,

    /**
     * @brief Memory persists for the lifetime of the current function (single threaded).
     */
    VD_MM_FUNCTION = 3,

    /**
     *  @brief Memory persist for the lifetime of the current entity.
     */
    VD_MM_ENTITY = 4,
} VD_MM_Tag;

typedef struct MemoryManager MemoryManager;

extern void GarbageCollectTask(struct ecs_iter_t *t);

#endif