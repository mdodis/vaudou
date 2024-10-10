#include "descriptor_allocator.h"

void vd_descriptor_allocator_init(
    VD_DescriptorAllocator *descalloc,
    VD_DescriptorAllocatorInitInfo *info)
{
    *descalloc = (VD_DescriptorAllocator){ 0 };

}

void vd_descriptor_allocator_deinit(VD_DescriptorAllocator *descalloc)
{

}