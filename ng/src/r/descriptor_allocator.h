#ifndef VD_DESCRIPTOR_ALLOCATOR_H
#define VD_DESCRIPTOR_ALLOCATOR_H
#include "vd_common.h"

#include "volk.h"

typedef struct {
    VkDescriptorType    type;
    float               ratio;
} VD_DescriptorPoolSizeRatio;

typedef struct {
    int b;
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

void vd_descriptor_allocator_deinit(VD_DescriptorAllocator *descalloc);

typedef struct {
    int b;
} VD_DescriptorWriter;

#endif // !VD_DESCRIPTOR_ALLOCATOR_H