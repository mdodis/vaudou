#ifndef VD_DELETION_QUEUE_H
#define VD_DELETION_QUEUE_H

#include "array.h"
#include "r/types.h"
#include "volk.h"

typedef struct {
    VkPipeline          pipeline;
    VkPipelineLayout    layout;
} VD_DeletionQueuePipelineAndLayout;

typedef struct {
    struct VD_Renderer                          *renderer;
    VD_ARRAY VkImage                            *images;
    VD_ARRAY VD_DeletionQueuePipelineAndLayout  *pipeline_and_layouts;
    VD_ARRAY VD_R_GPUMesh                       *meshes;
    VD_ARRAY VD(Buffer)                         *buffers;
    VD_ARRAY VD(Texture)                        *allocated_images;
    VD_Allocator                                allocator;
} VD_DeletionQueue;

typedef struct {
    struct VD_Renderer  *renderer;
    VD_Allocator        *allocator;
} VD_DeletionQueueInitInfo;

void vd_deletion_queue_init(VD_DeletionQueue *dq, VD_DeletionQueueInitInfo *info);
void vd_deletion_queue_push_pipeline_and_layout(
    VD_DeletionQueue *dq,
    VkPipeline pipeline,
    VkPipelineLayout layout);
void vd_deletion_queue_push_vkimage(VD_DeletionQueue *dq, VkImage image);
void vd_deletion_queue_push_image(VD_DeletionQueue *dq, VD(Texture) image);
void vd_deletion_queue_push_gpumesh(VD_DeletionQueue *dq, VD_R_GPUMesh *mesh);

void vd_deletion_queue_push_buffer(VD_DeletionQueue *dq, VD(Buffer) *buffer);

void vd_deletion_queue_flush(VD_DeletionQueue *dq);
void vd_deletion_queue_deinit(VD_DeletionQueue *dq);

#define VD_DELETION_QUEUE_PUSH(dq, v) _Generic((v), \
    VkImage: vd_deletion_queue_push_vkimage(dq, v)  \
)

#endif // !VD_DELETION_QUEUE_H
