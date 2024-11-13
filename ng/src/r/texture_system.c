#define VD_INTERNAL_SOURCE_FILE 1
#include "texture_system.h"
#include "vulkan_helpers.h"

static void free_texture(void *object, void *c);

int vd_texture_system_init(VD_R_TextureSystem *s, VD_R_TextureSystemInitInfo *info)
{
    VD_HANDLEMAP_INIT(s->image_handles, {
        .allocator = vd_memory_get_system_allocator(),
        .initial_capacity = 64,
        .on_free_object = free_texture,
        .c = s,
    });
    s->device = info->device;
    s->svma = info->svma;
    return 0;
}

Handle vd_texture_system_new(VD_R_TextureSystem *s, VD_R_TextureCreateInfo *info)
{
    Texture result = {};
    result.extent = info->extent;
    result.format = info->format;

    u32 mip_levels = 1;
    if (info->mipmapping.on) {
        mip_levels = floorf(log2f(glm_max(info->extent.width, info->extent.height))) + 1;
    }

    svma_create_texture(
        s->svma,
        & (VkImageCreateInfo)
        {
            .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .format         = info->format,
            .extent         = info->extent,
            .usage          = info->usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageType      = VK_IMAGE_TYPE_2D,
            .mipLevels      = mip_levels,
            .arrayLayers    = 1,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .tiling         = VK_IMAGE_TILING_OPTIMAL,

        },
        & (VmaAllocationCreateInfo)
        {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        },
        SVMA_CREATE_TRACKING(),
        &result.allocation,
        &result.image);

    VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (info->format == VK_FORMAT_D32_SFLOAT) {
        aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VD_VK_CHECK(vkCreateImageView(
        s->device,
        & (VkImageViewCreateInfo)
        {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format             = info->format,
            .image              = result.image,
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange   =
            {
                .aspectMask     = aspect_flags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        },
        0,
        &result.view));

    Handle result_handle = VD_HANDLEMAP_REGISTER(s->image_handles, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });

    return result_handle;
}

void vd_texture_system_deinit(VD_R_TextureSystem *s)
{
    VD_HANDLEMAP_DEINIT(s->image_handles);
}

static void free_texture(void *object, void *c)
{
    VD_R_TextureSystem *s = (VD_R_TextureSystem*)c;
    Texture *image = (Texture*)object;
    vkDestroyImageView(s->device, image->view, 0);
    svma_free_texture(s->svma, image->image, image->allocation);
}
