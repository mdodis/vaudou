#define VD_INTERNAL_SOURCE_FILE 1
#include "svma.h"
#include "vulkan_helpers.h"
#include "array.h"
#include <stdlib.h>

typedef struct {
    AllocationTracking  tracking;
    int                 used;
} ArrayEntry;

typedef struct SVMA {
    VkInstance                  instance;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;
    int                         track;
    VmaAllocator                allocator;
    VD_ARRAY ArrayEntry         **allocations;
} SVMA;

SVMA *svma_create()
{
    return (void*)calloc(1, sizeof(SVMA));
}

int svma_init(SVMA *s, SVMAInitInfo *info)
{
    s->device = info->device;
    s->instance = info->instance;
    s->physical_device = info->physical_device;
    s->track = info->track;

    if (s->track) {
        VD_LOG("SVMA", "Using VMA Tracking Mechanism");
        array_init(s->allocations, vd_memory_get_system_allocator());
    }

    VD_VK_CHECK(vmaCreateAllocator(
        & (VmaAllocatorCreateInfo)
        {
            .device                     = info->device,
            .physicalDevice             = info->physical_device,
            .instance                   = info->instance,
            .flags                      = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .pVulkanFunctions = & (VmaVulkanFunctions)
            {
                .vkGetInstanceProcAddr                  = vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr                    = vkGetDeviceProcAddr,
                .vkAllocateMemory                       = vkAllocateMemory,
                .vkBindBufferMemory                     = vkBindBufferMemory,
                .vkBindImageMemory                      = vkBindImageMemory,
                .vkCreateBuffer                         = vkCreateBuffer,
                .vkCreateImage                          = vkCreateImage,
                .vkDestroyBuffer                        = vkDestroyBuffer,
                .vkDestroyImage                         = vkDestroyImage,
                .vkFlushMappedMemoryRanges              = vkFlushMappedMemoryRanges,
                .vkFreeMemory                           = vkFreeMemory,
                .vkGetBufferMemoryRequirements          = vkGetBufferMemoryRequirements,
                .vkGetImageMemoryRequirements           = vkGetImageMemoryRequirements,
                .vkGetPhysicalDeviceMemoryProperties    = vkGetPhysicalDeviceMemoryProperties,
                .vkGetPhysicalDeviceProperties          = vkGetPhysicalDeviceProperties,
                .vkInvalidateMappedMemoryRanges         = vkInvalidateMappedMemoryRanges,
                .vkMapMemory                            = vkMapMemory,
                .vkUnmapMemory                          = vkUnmapMemory,
                .vkCmdCopyBuffer                        = vkCmdCopyBuffer,
            },
        },
        &s->allocator));
    return 0;
}

static void free_allocation(SVMA *s, Allocation allocation)
{
    if (s->track) {
        VmaAllocationInfo alloc_info;
        vmaGetAllocationInfo(s->allocator, (VmaAllocation)allocation.opaq, &alloc_info);
        ArrayEntry *entry_ptr = (ArrayEntry*)alloc_info.pUserData;

        entry_ptr->used = 0;
    }
}

int svma_create_buffer(
    SVMA *s,
    VkBufferCreateInfo *buffer_info,
    VmaAllocationCreateInfo *allocation_info,
    AllocationTracking *tracking,
    Allocation *result,
    VkBuffer *buffer)
{

    VD_VK_CHECK(vmaCreateBuffer(
        s->allocator,
        buffer_info,
        allocation_info,
        buffer,
        (VmaAllocation*)&result->opaq,
        0));

    if (s->track) {
        tracking->file = strdup(tracking->file);
        ArrayEntry *entry = memcpy(
            malloc(sizeof(ArrayEntry)),
            & (ArrayEntry)
            {
                .tracking = *tracking,
                .used = 1,
            },
            sizeof(ArrayEntry));

        array_add(s->allocations, entry);

        vmaSetAllocationUserData(s->allocator, (VmaAllocation)result->opaq, entry);
    }

    return 0;
}

void *svma_map(SVMA *s, Allocation allocation)
{
    void *result;
    VD_VK_CHECK(vmaMapMemory(s->allocator, (VmaAllocation)allocation.opaq, &result));
    return result;
}

void svma_unmap(SVMA *s, Allocation allocation)
{
    vmaUnmapMemory(s->allocator, (VmaAllocation)allocation.opaq);
}

size_t svma_get_size(SVMA *s, Allocation allocation)
{
    VmaAllocationInfo info;
    vmaGetAllocationInfo(s->allocator, (VmaAllocation)allocation.opaq, &info);
    return info.size;
}

void svma_free_buffer(
    SVMA *s,
    VkBuffer buffer,
    Allocation allocation)
{
    free_allocation(s, allocation);
    vmaDestroyBuffer(s->allocator, buffer, (VmaAllocation)allocation.opaq);
}

int svma_create_texture(
    SVMA *s,
    VkImageCreateInfo *image_info,
    VmaAllocationCreateInfo *allocation_info,
    AllocationTracking *tracking,
    Allocation *result,
    VkImage *image)
{
    VD_VK_CHECK(vmaCreateImage(
        s->allocator,
        image_info,
        allocation_info,
        image,
        (VmaAllocation*)&result->opaq,
        0));

    if (s->track) {
        tracking->file = strdup(tracking->file);
        ArrayEntry *entry = memcpy(
            malloc(sizeof(ArrayEntry)),
            & (ArrayEntry)
            {
                .tracking = *tracking,
                .used = 1,
            },
            sizeof(ArrayEntry));
        array_add(s->allocations, entry);

        vmaSetAllocationUserData(s->allocator, (VmaAllocation)result->opaq, entry);
    }

    return 0;
}

void svma_free_texture(
    SVMA *s,
    VkImage image,
    Allocation allocation)
{
    free_allocation(s, allocation);
    vmaDestroyImage(s->allocator, image, (VmaAllocation)allocation.opaq);
}

int svma_deinit(SVMA *s)
{
    for (size_t i = 0; i < array_len(s->allocations); ++i) {
        if (!s->allocations[i]->used) {
            continue;
        }

        VD_LOG_FMT(
            "SVMA",
            "Allocation at %{cstr}:%{i32} was not freed!",
            s->allocations[i]->tracking.file,
            s->allocations[i]->tracking.line);
    }

    vmaDestroyAllocator(s->allocator);
    return 0;
}
