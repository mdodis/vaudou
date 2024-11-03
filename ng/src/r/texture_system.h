#ifndef VD_R_TEXTURE_SYSTEM_H
#define VD_R_TEXTURE_SYSTEM_H
#include "handlemap.h"
#include "r/types.h"
#include "volk.h"
#include "vk_mem_alloc.h"
#include "r/svma.h"

typedef struct {
    VD_HANDLEMAP VD_R_AllocatedImage    *image_handles;
    VkDevice                            device;
    SVMA                                *svma;
} VD_R_TextureSystem;

typedef struct {
    VkDevice        device;
    SVMA            *svma;
} VD_R_TextureSystemInitInfo;

int vd_texture_system_init(VD_R_TextureSystem *s, VD_R_TextureSystemInitInfo *info);
HandleOf(VD_R_AllocatedImage) vd_texture_system_new(
    VD_R_TextureSystem *s,
    VD_R_TextureCreateInfo *info);
void vd_texture_system_deinit(VD_R_TextureSystem *s);

#endif // !VD_R_TEXTURE_SYSTEM_H
