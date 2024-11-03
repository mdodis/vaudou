#ifndef VD_SVMA_H
#define VD_SVMA_H
#include "r/types.h"

typedef struct SVMA SVMA;

typedef struct {
    VkInstance          instance;
    VkPhysicalDevice    physical_device;
    VkDevice            device;
    int                 track;
} SVMAInitInfo;

SVMA *svma_create();
int svma_init(SVMA *s, SVMAInitInfo *info);

int svma_create_buffer(
    SVMA *s,
    VkBufferCreateInfo *buffer_info,
    VmaAllocationCreateInfo *allocation_info,
    AllocationTracking *tracking,
    Allocation *result,
    VkBuffer *buffer);

void svma_free_buffer(
    SVMA *s,
    VkBuffer buffer,
    Allocation allocation);

int svma_create_texture(
    SVMA *s,
    VkImageCreateInfo *image_info,
    VmaAllocationCreateInfo *allocation_info,
    AllocationTracking *tracking,
    Allocation *result,
    VkImage *image);

void svma_free_texture(
    SVMA *s,
    VkImage image,
    Allocation allocation);

int svma_deinit(SVMA *s);

void *svma_map(SVMA *s, Allocation allocation);
void svma_unmap(SVMA *s, Allocation allocation);
size_t svma_get_size(SVMA *s, Allocation allocation);

#define SVMA_CREATE_TRACKING() & (AllocationTracking) \
    { \
        .file = __FILE__, \
        .line = __LINE__, \
    }

#endif // !VD_SVMA_H
