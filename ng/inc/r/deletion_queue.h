#ifndef VD_DELETION_QUEUE_H
#define VD_DELETION_QUEUE_H

#include "array.h"

#include "volk.h"

typedef struct {
    VkDevice                device;
    VD_ARRAY VkImage        *images;
    VD_Allocator            allocator;
} VD_DeletionQueue;

typedef struct {
    VkDevice            device;
    VD_Allocator        *allocator;
} VD_DeletionQueueInitInfo;

void vd_deletion_queue_init(VD_DeletionQueue *dq, VD_DeletionQueueInitInfo *info);
void vd_deletion_queue_push_vkimage(VD_DeletionQueue *dq, VkImage image);
void vd_deletion_queue_flush(VD_DeletionQueue *dq);
void vd_deletion_queue_deinit(VD_DeletionQueue *dq);

#define VD_DELETION_QUEUE_PUSH(dq, v) _Generic((v), \
    VkImage: vd_deletion_queue_push_vkimage(dq, v)  \
)

#endif // !VD_DELETION_QUEUE_H
