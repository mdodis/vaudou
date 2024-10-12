#ifndef VD_DESCRIPTOR_ALLOCATOR_H
#define VD_DESCRIPTOR_ALLOCATOR_H
#include "vd_common.h"
#include "array.h"

#include "volk.h"

typedef struct {
    VkDescriptorType    type;
    float               ratio;
} VD_DescriptorPoolSizeRatio;

typedef struct {
    VkDevice                            device;
    VD_ARRAY VD_DescriptorPoolSizeRatio *ratios;
    VD_ARRAY VkDescriptorPool           *full_pools;
    VD_ARRAY VkDescriptorPool           *free_pools;
    u32                                 sets_per_pool;
} VD_DescriptorAllocator;

typedef struct {
    VkDevice                    device;
    u32                         initial_sets;
    VD_DescriptorPoolSizeRatio  *ratios;
    u32                         num_ratios;
} VD_DescriptorAllocatorInitInfo;

void vd_descriptor_allocator_init(
    VD_DescriptorAllocator *descalloc,
    VD_DescriptorAllocatorInitInfo *info);

void vd_descriptor_allocator_clear(VD_DescriptorAllocator *descalloc);

VkDescriptorSet vd_descriptor_allocator_allocate(
    VD_DescriptorAllocator *descalloc,
    VkDescriptorSetLayout layout,
    void *pnext);

void vd_descriptor_allocator_deinit(VD_DescriptorAllocator *descalloc);

// ----WRITE DESCRIPTORS----------------------------------------------------------------------------
typedef enum {
    VD_R_DESCRIPTOR_SET_ENTRY_BUFFER,
    VD_R_DESCRIPTOR_SET_ENTRY_IMAGE,
} VD_R_DescriptorSetEntryType;

typedef struct {
    int                         binding;
    VD_R_DescriptorSetEntryType type;
    VkDescriptorType            t;
    union {
        VkDescriptorBufferInfo  buffer;
        VkDescriptorImageInfo   image;
    };
} VD_R_DescriptorSetEntry;

typedef struct {
    VkDevice                device;
    VD_R_DescriptorSetEntry *entries;
    u32                     num_entries;
} VD_R_WriteDescriptorSetsInfo;

void vd_r_write_descriptor_sets(
    VkDescriptorSet set,
    VD_R_WriteDescriptorSetsInfo *info,
    VD_Allocator *allocator);

#endif // !VD_DESCRIPTOR_ALLOCATOR_H
