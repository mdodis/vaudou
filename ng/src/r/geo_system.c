#include "geo_system.h"
#include "vulkan_helpers.h"

static void free_geo(void *object, void *c);

int vd_r_geo_system_init(VD_R_GeoSystem *s, VD_R_GeoSystemInitInfo *info)
{
    s->device = info->device;
    s->allocator = info->allocator;
    VD_HANDLEMAP_INIT(s->meshes, {
        .allocator = vd_memory_get_system_allocator(),
        .initial_capacity = 64,
        .on_free_object = free_geo,
        .c = s,
    });
    return 0;
}

HandleOf(VD_R_GPUMesh) vd_r_geo_system_new(VD_R_GeoSystem *s, VD_R_MeshCreateInfo *info)
{
    VD_R_GPUMesh result;
    VD_VK_CHECK(vmaCreateBuffer(
        s->allocator,
        & (VkBufferCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .size = info->num_indices * sizeof(u32),
        },
        & (VmaAllocationCreateInfo)
        {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        },
        &result.index.buffer,
        &result.index.allocation,
        &result.index.info));

    VD_VK_CHECK(vmaCreateBuffer(
        s->allocator,
        & (VkBufferCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .size = info->num_vertices * sizeof(VD_R_Vertex),
        },
        & (VmaAllocationCreateInfo)
        {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        },
        &result.vertex.buffer,
        &result.vertex.allocation,
        &result.vertex.info));

    result.vertex_buffer_address = vkGetBufferDeviceAddress(
        s->device,
        & (VkBufferDeviceAddressInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = result.vertex.buffer,
        });

    return VD_HANDLEMAP_REGISTER(s->meshes, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });
}

void vd_r_geo_system_deinit(VD_R_GeoSystem *s)
{
    VD_HANDLEMAP_DEINIT(s->meshes);
}

static void free_geo(void *object, void *c)
{
    VD_R_GeoSystem *s = (VD_R_GeoSystem*)c;
    VD_R_GPUMesh *m = (VD_R_GPUMesh*)object;

    vmaDestroyBuffer(s->allocator, m->index.buffer, m->index.allocation);
    vmaDestroyBuffer(s->allocator, m->vertex.buffer, m->vertex.allocation);
    m->vertex_buffer_address = 0;
}
