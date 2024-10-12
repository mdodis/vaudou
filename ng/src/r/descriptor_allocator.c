#define VD_INTERNAL_SOURCE_FILE 1
#include "r/descriptor_allocator.h"

#include "vulkan_helpers.h"
#include "array.h"
#include "mm.h"
#include "instance.h"

static VkDescriptorPool create_pool(
    VD_DescriptorAllocator *descalloc,
    u32 set_count,
    VD_DescriptorPoolSizeRatio *ratios,
    u32 num_ratios)
{
    dynarray VkDescriptorPoolSize *pool_sizes = 0;
    array_init(pool_sizes, VD_MM_FRAME_ALLOCATOR());

    for (u32 i = 0; i < num_ratios; ++i) {
        VkDescriptorPoolSize entry = {
            .type = ratios->type,
            .descriptorCount = (u32)(ratios->ratio * set_count),
        };

        array_add(pool_sizes, entry);
    }

    VkDescriptorPool new_pool;
    VD_VK_CHECK(vkCreateDescriptorPool(
        descalloc->device,
        &(VkDescriptorPoolCreateInfo)
        {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags          = 0,
            .maxSets        = set_count,
            .poolSizeCount  = array_len(pool_sizes),
            .pPoolSizes     = pool_sizes,
        },
        0,
        &new_pool));
    
    return new_pool;
}

static VkDescriptorPool get_pool(VD_DescriptorAllocator *descalloc)
{
    VkDescriptorPool result;
    if (array_len(descalloc->free_pools) != 0) {
        result = array_pop(descalloc->free_pools);
    } else {
        result = create_pool(
            descalloc,
            descalloc->sets_per_pool,
            descalloc->ratios,
            array_len(descalloc->ratios));

        descalloc->sets_per_pool = (u32)(descalloc->sets_per_pool * 1.5f);
        if (descalloc->sets_per_pool > 4092) {
            descalloc->sets_per_pool = 4092;
        }
    }

    return result;
}

void vd_descriptor_allocator_init(
    VD_DescriptorAllocator *descalloc,
    VD_DescriptorAllocatorInitInfo *info)
{
    *descalloc = (VD_DescriptorAllocator){ 0 };
    descalloc->device = info->device;
    descalloc->ratios       = 0;
    array_init(descalloc->ratios, VD_MM_GLOBAL_ALLOCATOR());
    
    descalloc->full_pools   = 0;
    array_init(descalloc->full_pools, VD_MM_GLOBAL_ALLOCATOR());
    
    descalloc->free_pools   = 0;
    array_init(descalloc->free_pools, VD_MM_GLOBAL_ALLOCATOR());

    descalloc->sets_per_pool = (u32)((float)info->initial_sets * 1.5f);
    
    for (u32 i = 0; i < info->num_ratios; ++i) {
        array_add(descalloc->ratios, info->ratios[i]);
    }

    VkDescriptorPool new_pool = create_pool(
        descalloc,
        info->initial_sets,
        descalloc->ratios,
        array_len(descalloc->ratios));

    array_add(descalloc->free_pools, new_pool);
}

VkDescriptorSet vd_descriptor_allocator_allocate(
    VD_DescriptorAllocator *descalloc,
    VkDescriptorSetLayout layout,
    void *pnext)
{
    VkDescriptorPool pool_to_use = get_pool(descalloc);
    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool_to_use,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
        .pNext = pnext,
    };

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(
        descalloc->device,
        &info,
        &ds);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        array_add(descalloc->full_pools, pool_to_use);
        pool_to_use = get_pool(descalloc);
        info.descriptorPool = pool_to_use;

        VD_VK_CHECK(vkAllocateDescriptorSets(
            descalloc->device,
            &info,
            &ds));
    }

    array_add(descalloc->free_pools, pool_to_use);

    return ds;
}

void vd_descriptor_allocator_clear(VD_DescriptorAllocator *descalloc)
{
    for (int i = 0; i < array_len(descalloc->free_pools); ++i) {
        vkResetDescriptorPool(descalloc->device, descalloc->free_pools[i], 0);
    }

    for (int i = 0; i < array_len(descalloc->full_pools); ++i) {
        vkResetDescriptorPool(descalloc->device, descalloc->full_pools[i], 0);
        array_add(descalloc->free_pools, descalloc->full_pools[i]);
    }

    array_clear(descalloc->full_pools);
}

void vd_descriptor_allocator_deinit(VD_DescriptorAllocator *descalloc)
{
    for (int i = 0; i < array_len(descalloc->free_pools); ++i) {
        vkDestroyDescriptorPool(descalloc->device, descalloc->free_pools[i], 0);
    }

    for (int i = 0; i < array_len(descalloc->full_pools); ++i) {
        vkDestroyDescriptorPool(descalloc->device, descalloc->full_pools[i], 0);
    }

    array_deinit(descalloc->free_pools);
    array_deinit(descalloc->full_pools);
}

void vd_r_write_descriptor_sets(
    VkDescriptorSet set,
    VD_R_WriteDescriptorSetsInfo *info,
    VD_Allocator *allocator)
{
    dynarray VkWriteDescriptorSet *writes = 0;
    array_init(writes, allocator);
    array_addn(writes, info->num_entries);

    for (u32 i = 0; i < info->num_entries; ++i) {
        VD_R_DescriptorSetEntry *entry = &info->entries[i];
        VkWriteDescriptorSet new_set = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = entry->binding,
            .dstSet = set,
            .descriptorType = entry->t,
            .descriptorCount = 1,
        };

        if (entry->type == VD_R_DESCRIPTOR_SET_ENTRY_BUFFER) {
            new_set.pBufferInfo = &entry->buffer;
        } else if (entry->type == VD_R_DESCRIPTOR_SET_ENTRY_IMAGE) {
            new_set.pImageInfo = &entry->image;
        }

        writes[i] = new_set;
    }

    vkUpdateDescriptorSets(info->device, array_len(writes), writes, 0, 0);

    array_deinit(writes);
}
