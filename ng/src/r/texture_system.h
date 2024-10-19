#ifndef VD_R_TEXTURE_SYSTEM_H
#define VD_R_TEXTURE_SYSTEM_H
#include "handlemap.h"
#include "r/types.h"
#include "volk.h"
#include "vk_mem_alloc.h"

typedef struct {
    VD_HANDLEMAP VD_R_AllocatedImage    *image_handles;
    VkDevice                            device;
    VmaAllocator                        allocator;
} VD_R_TextureSystem;

typedef struct {
    VkDevice        device;
    VmaAllocator    allocator;
} VD_R_TextureSystemInitInfo;

typedef struct {
    VkExtent3D          extent;
    VkFormat            format;
    VkImageUsageFlags   usage;
    struct {
        int on;
    } mipmapping;
} VD_R_TextureSystemNewInfo;

int vd_texture_system_init(VD_R_TextureSystem *s, VD_R_TextureSystemInitInfo *info);
VD_Handle vd_texture_system_new(VD_R_TextureSystem *s, VD_R_TextureSystemNewInfo *info);
void vd_texture_system_deinit(VD_R_TextureSystem *s);

#endif // !VD_R_TEXTURE_SYSTEM_H
