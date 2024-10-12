#define VD_INTERNAL_SOURCE_FILE 1
#include "r/deletion_queue.h"
#include "renderer.h"

void vd_deletion_queue_init(VD_DeletionQueue *dq, VD_DeletionQueueInitInfo *info)
{
    dq->allocator = *info->allocator;
    dq->renderer = info->renderer;

    dq->images = 0;
    dq->pipeline_and_layouts = 0;
    dq->meshes = 0;
    dq->buffers = 0;
    array_init(dq->images, &dq->allocator);
    array_init(dq->pipeline_and_layouts, &dq->allocator);
    array_init(dq->meshes, &dq->allocator);
    array_init(dq->buffers, &dq->allocator);
}

void vd_deletion_queue_push_pipeline_and_layout(
    VD_DeletionQueue *dq,
    VkPipeline pipeline,
    VkPipelineLayout layout)
{
    VD_DeletionQueuePipelineAndLayout pipeline_and_layout = {pipeline, layout};
    array_add(dq->pipeline_and_layouts, pipeline_and_layout);
}

void vd_deletion_queue_push_vkimage(VD_DeletionQueue *dq, VkImage image)
{
    array_add(dq->images, image);
}

void vd_deletion_queue_push_gpumesh(VD_DeletionQueue *dq, VD_R_GPUMesh *mesh)
{
    array_add(dq->meshes, *mesh);
}

void vd_deletion_queue_push_buffer(VD_DeletionQueue *dq, VD_R_AllocatedBuffer *buffer)
{
    array_add(dq->buffers, *buffer);
}

void vd_deletion_queue_flush(VD_DeletionQueue *dq)
{
    VkDevice device = vd_renderer_get_device(dq->renderer);

    for (int i = 0; i < array_len(dq->buffers); ++i) {
        vd_renderer_destroy_buffer(dq->renderer, &dq->buffers[i]);
    }
    array_clear(dq->buffers);

    for (int i = 0; i < array_len(dq->meshes); ++i) {
        vd_renderer_destroy_buffer(dq->renderer, &dq->meshes[i].index);
        vd_renderer_destroy_buffer(dq->renderer, &dq->meshes[i].vertex);
    }
    array_clear(dq->meshes);

    for (int i = 0; i < array_len(dq->pipeline_and_layouts); i++) {
        vkDestroyPipelineLayout(device, dq->pipeline_and_layouts[i].layout, 0);
        vkDestroyPipeline(device, dq->pipeline_and_layouts[i].pipeline, 0);
    }
    array_clear(dq->pipeline_and_layouts);

    for (int i = 0; i < array_len(dq->images); i++) {
        vkDestroyImage(device, dq->images[i], 0);
    }
    array_clear(dq->images);

}

void vd_deletion_queue_deinit(VD_DeletionQueue *dq)
{
    vd_deletion_queue_flush(dq);
}
