#ifndef VD_R_TYPES_H
#define VD_R_TYPES_H

#include "array.h"
#include "vd_common.h"
#include "volk.h"
#include "vk_mem_alloc.h"
#include "handlemap.h"
#include "vd_meta.h"

#include "cglm/cglm.h"
#include "cglm/quat.h"

enum {
    VD_MAX_SHADERS_PER_MATERIAL = 2,
    VD_MAX_BINDINGS_PER_SHADER  = 4,
    VD_(MAX_MATERIAL_PROPERTIES) = 8,
    VD_MAX_UNIFORM_BUFFERS_PER_MATERIAL = 3,
    VD_PASS_OPAQUE = 1000,
    VD_PASS_TRANSPARENT = 2000,
    VD_PASS_FINAL = 4000,
    VD_PASS_MAX = 100000,
};

typedef enum {
    VD_(SHADER_STAGE_VERT_BIT) = 1 << 0,
    VD_(SHADER_STAGE_FRAG_BIT) = 1 << 1,
    VD_(SHADER_STAGE_COMP_BIT) = 1 << 1,
} VD(ShaderStage);

typedef struct {
    uintptr_t opaq;
} VD(Allocation);

typedef struct {
    const char  *file;
    size_t      line;
} VD(AllocationTracking);

typedef struct {
    VkImage         image;
    VkImageView     view;
    VD(Allocation)  allocation;
    VkExtent3D      extent;
    VkFormat        format;
} VD_R_AllocatedImage; 

typedef struct {
    VkBuffer            buffer;
    VD(Allocation)      allocation;
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
    size_t num_vertices;
    size_t num_indices;
} VD_R_GPUMesh;

typedef struct {
    VkExtent3D          extent;
    VkFormat            format;
    VkImageUsageFlags   usage;
    struct {
        int on;
    } mipmapping;
} VD_R_TextureCreateInfo;

typedef struct {
    size_t          num_vertices;
    size_t          num_indices;
} VD_R_MeshCreateInfo;

typedef struct {
    HandleOf(VD_R_GPUMesh)  mesh;
    VD_R_Vertex             *vertices;
    size_t                  num_vertices;
    u32                     *indices;
    size_t                  num_indices;
} VD_R_MeshWriteInfo;

typedef struct {
    mat4 view;
    mat4 proj;
    vec3 sun_direction;
    vec3 sun_color;
} VD_R_SceneData;

// ----SHADERS--------------------------------------------------------------------------------------
typedef struct {
    VkShaderModule      module;
    VkShaderStageFlags  stage;
} GPUShader;

typedef struct {
    VkShaderStageFlags  stage;
    void                *bytecode;
    size_t              bytecode_size;
    const char          *sourcecode;
    size_t              sourcecode_len;
} GPUShaderCreateInfo;

// ----MATERIALS------------------------------------------------------------------------------------
typedef enum {
    BINDING_TYPE_INVALID = 0,
    BINDING_TYPE_SAMPLER2D,
    BINDING_TYPE_STRUCT,
} BindingType;

typedef struct {
    BindingType type;
    size_t      struct_size;
} BindingInfo;

typedef enum {
    VD_(PUSH_CONSTANT_TYPE_DEFAULT) = 0,
    VD_(PUSH_CONSTANT_TYPE_CUSTOM) = 0xFF,
} VD(PushConstantType);

typedef struct {
    VD(PushConstantType) type;
    size_t               size;
    VD(ShaderStage)      stage;
} VD(PushConstantInfo);

typedef struct {
    VkDeviceAddress vertex_address;
    mat4            obj;
} VD(DefaultPushConstant);

typedef struct {
    VD(PushConstantInfo) info;
    union {
        void                    *ptr;
        VD(DefaultPushConstant) def;
    };
} VD(PushConstant);

static VD_INLINE void *vd(get_push_constant_ptr)(VD(PushConstant) *pc)
{
    if (pc->info.type == VD(PUSH_CONSTANT_TYPE_DEFAULT)) {
        return &pc->def;
    }

    return pc->ptr;
}

typedef struct {
    BindingInfo binding;
    union {
        void                            *pstruct;
        HandleOf(VD_R_AllocatedImage)   sampler2d;
    };
} VD(MaterialProperty);

typedef struct {
    HandleOf(VD_R_GPUShader) shaders[VD_MAX_SHADERS_PER_MATERIAL];
    u32                      num_shaders;
    VkPrimitiveTopology      topology;
    VkPolygonMode            polygon_mode;
    VkCullModeFlags          cull_mode;
    VkFrontFace              cull_face;
    int                      pass;
    int                      num_vertex_attributes;
    struct {
        int                  custom;
        VD(PushConstantInfo) info;
    } push_constant;
    struct {
        int                  on;
    } multisample;

    struct {
        int                  on;
    } blend;

    struct {
        int                  on;
        int                  write;
        VkCompareOp          cmp_op;
    } depth_test;

    u32                      num_properties;
    VD(MaterialProperty)     *properties;
} MaterialBlueprint;

typedef struct {
    VkPipeline                      pipeline;
    VkPipelineLayout                layout;
    VkDescriptorSetLayout           property_layout;
    u32                             num_properties;
    VD(MaterialProperty)            properties[VD_(MAX_MATERIAL_PROPERTIES)];
    VD(PushConstantInfo)            push_constant_info;
} VD(GPUMaterialBlueprint);

typedef struct {
    HandleOf(VD(GPUMaterialBlueprint))  blueprint;
    u32                                 num_buffers;
    VD_R_AllocatedBuffer                buffers[VD_MAX_UNIFORM_BUFFERS_PER_MATERIAL];
    VD(MaterialProperty)                properties[VD_(MAX_MATERIAL_PROPERTIES)];
} VD(GPUMaterial);

typedef struct {
    HandleOf(GPUMaterial)   material;
    u32                     num_properties;
    MaterialProperty        *properties;
} MaterialWriteInfo;

typedef struct {
    HandleOf(GPUMaterial)   material;
    int                     pass;
    VkDescriptorSet         default_set;
    VkDescriptorSet         property_set;
} GPUMaterialInstance;

typedef struct {
    HandleOf(VD_R_GPUMesh)  mesh;
    HandleOf(GPUMaterial)   material;
    VD(PushConstant)        push_constant;
    u32                     index_count;
    u32                     first_index;
} VD(RenderObject);


void vd_r_generate_sphere_data(
    VD_R_Vertex **vertices,
    int *num_vertices,
    unsigned int **indices,
    int *num_indices,
    float radius,
    int segments,
    int rings,
    VD_Allocator *allocator);

void vd_r_generate_cube_data(
    VD_R_Vertex **vertices,
    int *num_vertices,
    unsigned int **indices,
    int *num_indices,
    vec3 extents,
    VD_Allocator *allocator);

void *vd_r_generate_checkerboard(
    u32 even_color,
    u32 odd_color,
    int width,
    int height,
    size_t *size,
    VD_Allocator *allocator);

void vd_r_perspective(mat4 proj, float fov, float aspect, float znear, float zfar);

#endif // !VD_R_TYPES_H
