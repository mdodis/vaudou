#ifndef VD_R_TYPES_H
#define VD_R_TYPES_H

#include "volk.h"
#include "vk_mem_alloc.h"

typedef struct {
    VkImage         image;
    VkImageView     view;
    VmaAllocation   allocation;
    VkExtent3D      extent;
    VkFormat        format;
} VD_R_AllocatedImage; 


#endif // !VD_R_TYPES_H