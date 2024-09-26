#define VD_INTERNAL_SOURCE_FILE 1
#include "deletion_queue.h"

void vd_deletion_queue_init(VD_DeletionQueue *dq, VD_DeletionQueueInitInfo *info)
{
    dq->device = info->device;
    dq->allocator = *info->allocator;

    dq->images = 0;
    array_init(dq->images, &dq->allocator);
}

void vd_deletion_queue_push_vkimage(VD_DeletionQueue *dq, VkImage image)
{
    array_add(dq->images, image);
}

void vd_deletion_queue_flush(VD_DeletionQueue *dq)
{
    for (int i = 0; i < array_len(dq->images); i++) {
        vkDestroyImage(dq->device, dq->images[i], 0);
    }

    array_clear(dq->images);
}

void vd_deletion_queue_deinit(VD_DeletionQueue *dq)
{
    vd_deletion_queue_flush(dq);
}