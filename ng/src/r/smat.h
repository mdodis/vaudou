#ifndef VD_SMAT_H
#define VD_SMAT_H
#include "r/types.h"
#include "r/descriptor_allocator.h"
#include "r/svma.h"

typedef struct {
    VD_HANDLEMAP GPUMaterial            *materials;
    VD_HANDLEMAP GPUMaterialBlueprint   *blueprints;
    VkDevice                 device;
    SVMA                     *svma;
    VkDescriptorSetLayout    set0_layout;
    VD_DescriptorAllocator   *desc_allocator;
    VD_R_AllocatedBuffer     set0_buffers[VD_MAX_UNIFORM_BUFFERS_PER_MATERIAL];
    u32                      num_set0_buffers;
    struct {
        VkSampler linear;
    } samplers;

    VkFormat                color_format;
    VkFormat                depth_format;
    VD(PushConstantInfo)    default_push_constant;
} SMat;

typedef struct {
    VkDevice                device;
    SVMA                    *svma;

    u32                     num_set0_bindings;
    BindingInfo             *set0_bindings;
    VD(PushConstantInfo)    default_push_constant;

    VkFormat                color_format;
    VkFormat                depth_format;
} SMatInitInfo;

int smat_init(SMat *s, SMatInitInfo *info);

HandleOf(GPUMaterialBlueprint) smat_new_blueprint(SMat *s, MaterialBlueprint *b);
HandleOf(GPUMaterial) smat_new_from_blueprint(SMat *s, HandleOf(GPUMaterialBlueprint) b);

void smat_begin_frame(SMat *s, VD_DescriptorAllocator *descriptor_allocator);

GPUMaterialInstance smat_prep(SMat *s, MaterialWriteInfo *set0_info);
void smat_end_frame(SMat *s);
void smat_deinit(SMat *s);

#endif // !VD_SMAT_H
