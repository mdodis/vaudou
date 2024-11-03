#include "geo_system.h"
#include "vulkan_helpers.h"

static void free_geo(void *object, void *c);

int vd_r_geo_system_init(VD_R_GeoSystem *s, VD_R_GeoSystemInitInfo *info)
{
    s->device = info->device;
    s->svma = info->svma;
    VD_HANDLEMAP_INIT(s->meshes, {
        .allocator = vd_memory_get_system_allocator(),
        .initial_capacity = 64,
        .on_free_object = free_geo,
        .c = s,
    });
    return 0;
}

static void create_buffers(VD_R_GeoSystem *s, VD_R_MeshCreateInfo *info, VD_R_GPUMesh *result)
{
    svma_create_buffer(
        s->svma,
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
        SVMA_CREATE_TRACKING(),
        &result->index.allocation,
        &result->index.buffer);

    svma_create_buffer(
        s->svma,
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
        SVMA_CREATE_TRACKING(),
        &result->vertex.allocation,
        &result->vertex.buffer);

    result->vertex_buffer_address = vkGetBufferDeviceAddress(
        s->device,
        & (VkBufferDeviceAddressInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = result->vertex.buffer,
        });
}

HandleOf(VD_R_GPUMesh) vd_r_geo_system_new(VD_R_GeoSystem *s, VD_R_MeshCreateInfo *info)
{
    VD_R_GPUMesh result;

    create_buffers(s, info, &result);

    return VD_HANDLEMAP_REGISTER(s->meshes, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });
}

int sgeo_resize(VD_R_GeoSystem *s, HandleOf(VD_R_GPUMesh) mesh, VD_R_MeshCreateInfo *info)
{
   VD_R_GPUMesh *meshptr = USE_HANDLE(mesh, VD_R_GPUMesh); 
   create_buffers(s, info, meshptr);
   return 0;
}

void vd_r_geo_system_deinit(VD_R_GeoSystem *s)
{
    VD_HANDLEMAP_DEINIT(s->meshes);
}

static void free_geo(void *object, void *c)
{
    VD_R_GeoSystem *s = (VD_R_GeoSystem*)c;
    VD_R_GPUMesh *m = (VD_R_GPUMesh*)object;

    svma_free_buffer(s->svma, m->index.buffer, m->index.allocation);
    svma_free_buffer(s->svma, m->vertex.buffer, m->vertex.allocation);
    m->vertex_buffer_address = 0;
}
