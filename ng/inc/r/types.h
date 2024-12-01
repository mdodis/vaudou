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
    VD_(MAX_SINKS) = 6,
    VD_(MAX_SOURCES) = 6,
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

typedef enum {
    VD_(SIZE_CLASS_UNDEFINED) = 0,
    VD_(SIZE_CLASS_SWAPCHAIN_RELATIVE),
    VD_(SIZE_CLASS_CUSTOM),
} VD(SizeClass);

typedef enum {
    VD_(FORMAT_UNDEFINED) = 0,
    VD_(FORMAT_R8G8B8A8_UNORM),
    VD_(FORMAT_R16G16B16A16_SFLOAT),
    VD_(FORMAT_B8G8R8A8_UNORM),
    VD_(FORMAT_D32_SFLOAT),
} VD(Format);

static VD_INLINE int format_is_depth_format(VD(Format) format)
{
    switch (format) {
        case VD_(FORMAT_D32_SFLOAT):
            return 1;
        default:
            return 0;
    }
}

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
} VD(Texture); 

typedef struct {
    VkBuffer            buffer;
    VD(Allocation)      allocation;
} VD(Buffer);

typedef struct {
    vec3    position;
    float   uv_x;
    vec3    normal;
    float   uv_y;
    vec4    color;
} VD_R_Vertex;

typedef struct {
    VD(Buffer)          vertex;
    VD(Buffer)          index;
    VkDeviceAddress     vertex_buffer_address;
    size_t              num_vertices;
    size_t              num_indices;
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
    VD(Buffer)                          buffers[VD_MAX_UNIFORM_BUFFERS_PER_MATERIAL];
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
    struct {
        int                 use_custom;
        vec4                custom;
    } scissor;
} VD(RenderObject);

// ----COMMANDS-------------------------------------------------------------------------------------

typedef enum {
    VD_(COMMAND_UNKNOWN) = 0,
    VD_(COMMAND_CLEAR_COLOR),
    VD_(COMMAND_BEGIN_RENDERING),
    VD_(COMMAND_END_RENDERING),
    VD_(COMMAND_SET_VIEWPORT),
    VD_(COMMAND_SET_SCISSOR),
    VD_(COMMAND_COPY_BUFFER),
    VD_(COMMAND_COPY_BUFFER_TO_TEXTURE),
    VD_(COMMAND_BIND_BLUEPRINT),
    VD_(COMMAND_WRITE_PUSH_CONSTANT),
    VD_(COMMAND_BIND_PROPERTIES),
    VD_(COMMAND_BIND_MESH),
    VD_(COMMAND_DRAW_INDEXED),
} VD(CommandType);

typedef enum {
    VD_(LOAD_OP_DONT_CARE),
    VD_(LOAD_OP_CLEAR),
    VD_(LOAD_OP_LOAD),
} VD(LoadOp);

typedef enum {
    VD_(STORE_OP_DONT_CARE),
    VD_(STORE_OP_STORE),
} VD(StoreOp);

typedef struct {
    Texture     *target;
    VD(LoadOp)  load_op;
    VD(StoreOp) store_op;
} VD(RenderingAttachmentInfo);

typedef struct {
    VD(CommandType) type;

    union {
        struct {
            Texture *texture;
            vec4    value;
        } clear_color;

        struct {
            vec4    area;
            VD(RenderingAttachmentInfo) color_attachment;
            VD(RenderingAttachmentInfo) depth_attachment;
        } begin_rendering;

        struct {
            float x;
            float y;
            float w;
            float h;
            float mind;
            float maxd;
        } set_viewport;

        struct {
            float x;
            float y;
            float w;
            float h;
        } set_scissor;

        struct {
            Buffer *src;
            size_t src_offset;
            Buffer *dst;
            size_t dst_offset;
            size_t size;
        } copy_buffer;

        struct {
            Buffer  *src;
            size_t  src_offset;
            size_t  src_row_length;
            Texture *tex;
            size_t  tex_height;
            ivec3   tex_extent;
        } copy_buffer_to_texture;

        struct {
            HandleOf(GPUMaterialBlueprint) blueprint;
        } bind_blueprint;

        struct {
            VD(ShaderStage)     stage;
            size_t              size;
            void                *data;
        } write_push_constant;

        struct {
            HandleOf(GPUMaterial) material;
        } bind_properties;

        struct {
            HandleOf(VD_R_GPUMesh) mesh;
        } bind_mesh;

        struct {
            u32 index_count;
            u32 instance_count;
            u32 first_index;
            u32 vertex_offset;
            u32 first_instance;
        } draw_indexed;
    };
} VD(Command);

// ----RENDER PASSES--------------------------------------------------------------------------------

typedef struct {
    void *opaque_ptr;
} VD(FrameData);

typedef struct {
    VD(SizeClass)   klass;
    union {
        vec2        relative;
        ivec2       absolute;
    };
} VD(Size);

typedef struct {
    VD(Format)      format;
    VD(Size)        size;
} VD(AttachmentInfo);

typedef enum {
    VD_(ORIGIN_FROM_SINK),
    VD_(ORIGIN_INTERNAL),
} VD(Origin);

typedef struct {
    const char          *name;
    VD(AttachmentInfo)  attachment_info;
    VD(Origin)          origin;

    HandleOf(Texture) runtime_image;
} VD(Source);

typedef struct {
    const char          *name;
    VD(AttachmentInfo)  attachment_info;
    VD(Source)          *binding;
} VD(Sink);

typedef struct VD(Pass) VD(Pass);

struct VD(Pass) {
    const char     *name;

    u32            num_sinks;
    VD(Sink)       sinks[VD_(MAX_SINKS)];

    u32            num_sources;
    VD(Source)     sources[VD_(MAX_SOURCES)];

    int (*init)(VD(Pass) *self);
    int (*run)(VD(Pass) *self, VD(FrameData) *frame_data);
    int (*deinit)(VD(Pass) *self);
};

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
