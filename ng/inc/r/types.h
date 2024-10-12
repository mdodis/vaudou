#ifndef VD_R_TYPES_H
#define VD_R_TYPES_H

#include "vd_common.h"
#include "volk.h"
#include "vk_mem_alloc.h"

#include "cglm/cglm.h"

typedef struct {
    VkImage         image;
    VkImageView     view;
    VmaAllocation   allocation;
    VkExtent3D      extent;
    VkFormat        format;
} VD_R_AllocatedImage; 

typedef struct {
    VkBuffer            buffer;
    VmaAllocation       allocation;
    VmaAllocationInfo   info;
} VD_R_AllocatedBuffer;

typedef struct {
    vec3    position;
    float   uv_x;
    vec3    normal;
    float   uv_y;
    vec4    color;
} VD_R_Vertex;

typedef struct {
    VD_R_AllocatedBuffer    vertex;
    VD_R_AllocatedBuffer    index;
    VkDeviceAddress         vertex_buffer_address;
} VD_R_GPUMesh;

typedef struct {
    mat4            world_matrix;
    VkDeviceAddress vertex_buffer;
} VD_R_GPUPushConstants;

typedef struct {
    mat4    view;
    mat4    proj;
    mat4    viewproj;
} VD_R_GPuSceneData;

void vd_r_generate_sphere_data(
    VD_R_Vertex **vertices,
    int *num_vertices,
    unsigned int **indices,
    int *num_indices,
    float radius,
    int segments,
    int rings,
    VD_Allocator *allocator);

void vd_r_perspective(mat4 proj, float fov, float aspect, float znear, float zfar);

#endif // !VD_R_TYPES_H
