#ifndef VD_SMAT_H
#define VD_SMAT_H
#include "r/types.h"
#include "r/descriptor_allocator.h"

typedef struct {
    VD_HANDLEMAP GPUMaterial *materials;
    VkDevice                 device;
    VmaAllocator             allocator;
    VkDescriptorSetLayout    set0_layout;
    VD_DescriptorAllocator   *desc_allocator;
    VD_R_AllocatedBuffer     set0_buffers[VD_MAX_UNIFORM_BUFFERS_PER_MATERIAL];
    u32                      num_set0_buffers;
    struct {
        VkSampler linear;
    } samplers;

    VkFormat                color_format;
    VkFormat                depth_format;
} SMat;

typedef struct {
    VkDevice                device;
    VmaAllocator            allocator;

    u32                     num_set0_bindings;
    BindingInfo             *set0_bindings;

    VkFormat                color_format;
    VkFormat                depth_format;
} SMatInitInfo;

int smat_init(SMat *s, SMatInitInfo *info);
HandleOf(GPUMaterial) smat_new(SMat *s, MaterialBlueprint *b);
void smat_begin_frame(SMat *s, VD_DescriptorAllocator *descriptor_allocator);

GPUMaterialInstance smat_write(SMat *s, MaterialWriteInfo *info, MaterialWriteInfo *set0_info);
void smat_end_frame(SMat *s);
void smat_deinit(SMat *s);

#endif // !VD_SMAT_H
