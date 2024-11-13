#define VD_INTERNAL_SOURCE_FILE 1
#include "handlemap.h"
#include "array.h"
#include "smat.h"
#include "vulkan_helpers.h"
#include "mm.h"
#include "instance.h"
#include "vd_vk.h"

static void free_material(void *object, void *c);
static void free_material_blueprint(void *object, void *c);

static VkDescriptorType binding_type_to_vk_descriptor_type(BindingType t);

int smat_init(SMat *s, SMatInitInfo *info)
{
    s->device = info->device;
    s->svma = info->svma;
    s->color_format = info->color_format;
    s->depth_format = info->depth_format;

    dynarray VkDescriptorSetLayoutBinding *bindings = 0;
    array_init(bindings, VD_MM_FRAME_ALLOCATOR());

    for (u32 i = 0 ; i < info->num_set0_bindings; ++i) {
        VkDescriptorSetLayoutBinding new_binding = {
            .descriptorType = binding_type_to_vk_descriptor_type(info->set0_bindings[i].type),
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        array_add(bindings, new_binding);
    }

    VD_VK_CHECK(vkCreateDescriptorSetLayout(
        s->device,
        & (VkDescriptorSetLayoutCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = info->num_set0_bindings,
            .pBindings = bindings,
        },
        0,
        &s->set0_layout));

    VD_VK_CHECK(vkCreateSampler(
        s->device,
        & (VkSamplerCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
        },
        0,
        &s->samplers.linear));

    // Create Uniform Buffers
    int buffer_index = 0;
    for (int i = 0; i < info->num_set0_bindings; ++i) {
        if (info->set0_bindings[i].type != BINDING_TYPE_STRUCT) {
            continue;
        }

        svma_create_buffer(
            s->svma,
            & (VkBufferCreateInfo)
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = info->set0_bindings[i].struct_size,
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            },
            & (VmaAllocationCreateInfo)
            {
                .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            },
            SVMA_CREATE_TRACKING(),
            &s->set0_buffers[buffer_index].allocation,
            &s->set0_buffers[buffer_index].buffer);

    }
    s->num_set0_buffers = buffer_index + 1;

    s->default_push_constant = info->default_push_constant;

    VD_HANDLEMAP_INIT(s->materials, {
        .initial_capacity = 64,
        .c = s,
        .allocator = vd_memory_get_system_allocator(),
        .on_free_object = free_material,
    });

    VD_HANDLEMAP_INIT(s->blueprints, {
        .initial_capacity = 64,
        .c = s,
        .allocator = vd_memory_get_system_allocator(),
        .on_free_object = free_material_blueprint,
    });
    return 0;
}

static void alloc_copy_or_zero_properties(
    MaterialProperty *src,
    int num_src,
    MaterialProperty *dst)
{
    memcpy(dst, src, num_src * sizeof(*src));

    for (int i = 0; i < num_src; ++i) {
        if (src[i].binding.type == BINDING_TYPE_STRUCT) {
            dst[i].pstruct = vd_malloc(
                VD_MM_GLOBAL_ALLOCATOR(),
                src[i].binding.struct_size);

            if (src[i].pstruct == 0) {
                memcpy(
                    dst[i].pstruct,
                    src[i].pstruct,
                    src[i].binding.struct_size);
            } else {
                memset(dst[i].pstruct, 0, dst[i].binding.struct_size);
            }
        }
    }
}

HandleOf(GPUMaterial) smat_new_from_blueprint(SMat *s, HandleOf(GPUMaterialBlueprint) b)
{
    GPUMaterial result = {0};
    GPUMaterialBlueprint *blueprint = USE_HANDLE(b, GPUMaterialBlueprint);

    result.blueprint = COPY_HANDLE(b);

    alloc_copy_or_zero_properties(
        blueprint->properties,
        blueprint->num_properties,
        result.properties);

    // Create Uniform Buffers
    int buffer_index = 0;
    for (int i = 0; i < blueprint->num_properties; ++i) {
        if (blueprint->properties[i].binding.type != BINDING_TYPE_STRUCT) {
            continue;
        }

        svma_create_buffer(
            s->svma,
            & (VkBufferCreateInfo)
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = blueprint->properties[i].binding.struct_size,
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            },
            & (VmaAllocationCreateInfo)
            {
                .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            },
            SVMA_CREATE_TRACKING(),
            &result.buffers[buffer_index].allocation,
            &result.buffers[buffer_index].buffer);

    }
    return VD_HANDLEMAP_REGISTER(s->materials, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });
}

HandleOf(GPUMaterialBlueprint) smat_new_blueprint(SMat *s, MaterialBlueprint *b)
{
    GPUMaterialBlueprint result;

    result.push_constant_info = b->push_constant.info;
    if (!b->push_constant.custom) {
        result.push_constant_info = s->default_push_constant;
    }

    dynarray VkDescriptorSetLayoutBinding *bindings = 0;
    array_init(bindings, VD_MM_FRAME_ALLOCATOR());
    array_addn(bindings, b->num_properties);

    alloc_copy_or_zero_properties(b->properties, b->num_properties, result.properties);
    result.num_properties = b->num_properties;

    for (u32 bb = 0; bb < b->num_properties; ++bb) {
        BindingInfo *binfo = &b->properties[bb].binding;

        bindings[bb] = (VkDescriptorSetLayoutBinding)
        {
            .binding = 0,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .descriptorCount = 1,
            .descriptorType = binding_type_to_vk_descriptor_type(binfo->type),
        };
    }

    VkDescriptorSetLayoutCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = array_len(bindings),
        .pBindings = bindings,
    };

    VD_VK_CHECK(vkCreateDescriptorSetLayout(
        s->device,
        &create_info,
        0,
        &result.property_layout));

    VkDescriptorSetLayout set_layouts[2] = {
        s->set0_layout,
        result.property_layout,
    };


    VkPushConstantRange push_constant_range = {
        .offset = 0,
        .stageFlags = vd_shader_stage_to_vk_shader_stage(result.push_constant_info.stage),
        .size = result.push_constant_info.size,
    };

    VD_VK_CHECK(vkCreatePipelineLayout(
        s->device,
        & (VkPipelineLayoutCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = VD_ARRAY_COUNT(set_layouts),
            .pSetLayouts = set_layouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = (VkPushConstantRange[]) {
                push_constant_range,
            },
            .pNext = 0,
        },
        0,
        &result.layout));

    // Populate shader stage info struct
    dynarray VkPipelineShaderStageCreateInfo *shader_stages = 0;
    array_init(shader_stages, VD_MM_FRAME_ALLOCATOR());
    array_addn(shader_stages, b->num_shaders);

    for (int i = 0; i < b->num_shaders; ++i) {
        shader_stages[i] = (VkPipelineShaderStageCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = USE_HANDLE(b->shaders[i], GPUShader)->module,
            .stage = USE_HANDLE(b->shaders[i], GPUShader)->stage,
            .pNext = 0,
            .pName = "main",
        };
    }

    VD_VK_CHECK(vd_vk_build_pipeline(
        s->device,
        & (VD_VK_PipelineBuildInfo)
        {
            .layout = result.layout,
            .blend = {
                .on = b->blend.on,
            },
            .num_stages = array_len(shader_stages),
            .stages = shader_stages,
            .topology = b->topology,
            .cull_mode = b->cull_mode,
            .front_face = b->cull_face,
            .depth_test = {
                .on = b->depth_test.on,
                .write = b->depth_test.write,
                .cmp_op = b->depth_test.cmp_op,
            },
            .multisample.on = b->multisample.on,
            .polygon_mode = b->polygon_mode,
            .color_format = s->color_format,
            .depth_format = s->depth_format,
        },
        &result.pipeline));


    return VD_HANDLEMAP_REGISTER(s->blueprints, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });
}

static VkDescriptorSet write_set(
    SMat *s,
    int n,
    u32 num_buffers,
    VD(Buffer) *buffers,
    VkDescriptorSetLayout set_layout,
    MaterialWriteInfo *info)
{
    int buffer_index = 0;
    VkDescriptorSet return_set = vd_descriptor_allocator_allocate(
        s->desc_allocator,
        set_layout,
        0);

    dynarray VkWriteDescriptorSet *writes = 0;
    array_init(writes, VD_MM_FRAME_ALLOCATOR());
    array_addn(writes, info->num_properties);

    dynarray VkDescriptorBufferInfo *buffer_infos = 0;
    array_init(buffer_infos, VD_MM_FRAME_ALLOCATOR());

    dynarray VkDescriptorImageInfo *image_infos = 0;
    array_init(image_infos, VD_MM_FRAME_ALLOCATOR());

    for (int i = 0; i < info->num_properties; ++i) {
        MaterialProperty *p = &info->properties[i];

        writes[i] = (VkWriteDescriptorSet) {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = return_set,
            .dstBinding = i,
            .descriptorType = binding_type_to_vk_descriptor_type(p->binding.type),
            .descriptorCount = 1,
        };

        switch (p->binding.type) {
            case BINDING_TYPE_STRUCT: {
                VD(Buffer) *buffer = &buffers[buffer_index++];
                void *data = svma_map(s->svma, buffer->allocation);
                memcpy(data, p->pstruct, p->binding.struct_size);

                svma_unmap(s->svma, buffer->allocation);

                VkDescriptorBufferInfo *binfo = array_addp(buffer_infos);
                binfo->buffer = buffer->buffer;
                binfo->offset = 0;
                binfo->range = p->binding.struct_size;

                writes[i].pBufferInfo = binfo;
            } break;
            case BINDING_TYPE_SAMPLER2D: {
                Texture *img = USE_HANDLE(p->sampler2d, Texture);
                VkDescriptorImageInfo *iinfo = array_addp(image_infos);
                iinfo->imageView = img->view;
                iinfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                iinfo->sampler = s->samplers.linear;

                writes[i].pImageInfo = iinfo;
            } break;
            default: break;
        }
    }

    vkUpdateDescriptorSets(
        s->device,
        array_len(writes),
        writes,
        0,
        0);

    return return_set;
}

void smat_begin_frame(SMat *s, VD_DescriptorAllocator *descriptor_allocator)
{
    s->desc_allocator = descriptor_allocator;
    vd_descriptor_allocator_clear(s->desc_allocator);
}

GPUMaterialInstance smat_prep(SMat *s, MaterialWriteInfo *set0_info)
{
    VkDescriptorSet set0 = write_set(
        s,
        0,
        s->num_set0_buffers,
        s->set0_buffers,
        s->set0_layout,
        set0_info);

    GPUMaterial *material = USE_HANDLE(set0_info->material, GPUMaterial);
    GPUMaterialBlueprint *blueprint = USE_HANDLE(material->blueprint, GPUMaterialBlueprint);
    int buffer_index = 0;
    VkDescriptorSet property_set = write_set(
        s,
        1,
        material->num_buffers,
        material->buffers,
        blueprint->property_layout,
        & (MaterialWriteInfo)
        {
            .material = set0_info->material,
            .num_properties = blueprint->num_properties,
            .properties = material->properties,
        });
    return (GPUMaterialInstance) {
        .default_set = set0,
        .property_set = property_set,
        .pass = 0,
        .material = set0_info->material,
    };
}

void smat_end_frame(SMat *s)
{
    s->desc_allocator = 0;
}

void smat_deinit(SMat *s)
{
    for (int i = 0; i < s->num_set0_buffers; ++i) {
        svma_free_buffer(
            s->svma,
            s->set0_buffers[i].buffer,
            s->set0_buffers[i].allocation);
    }

    vkDestroyDescriptorSetLayout(s->device, s->set0_layout, 0);
    vkDestroySampler(s->device, s->samplers.linear, 0);
    VD_HANDLEMAP_DEINIT(s->materials);
    VD_HANDLEMAP_DEINIT(s->blueprints);
}

static void free_material_blueprint(void *object, void *c)
{
    SMat *s = (SMat*)c;
    GPUMaterialBlueprint *blueprint = (GPUMaterialBlueprint*)object;

    vkDestroyDescriptorSetLayout(s->device, blueprint->property_layout, 0);
    vkDestroyPipeline(s->device, blueprint->pipeline, 0);
    vkDestroyPipelineLayout(s->device, blueprint->layout, 0);
}

static void free_material(void *object, void *c)
{
    SMat *s = (SMat*)c;
    GPUMaterial *material = (GPUMaterial*)object;

    for (int i = 0; i < material->num_buffers; ++i) {
        svma_free_buffer(
            s->svma,
            material->buffers[i].buffer,
            material->buffers[i].allocation);
    }

    DROP_HANDLE(material->blueprint);
}

static VkDescriptorType binding_type_to_vk_descriptor_type(BindingType t)
{
    switch (t) {
        case BINDING_TYPE_INVALID:      return 0;
        case BINDING_TYPE_SAMPLER2D:    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case BINDING_TYPE_STRUCT:       return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default: return 0;
    }
}

VkShaderStageFlags vd_shader_stage_to_vk_shader_stage(ShaderStage stage)
{
    VkShaderStageFlags result = 0;

    if (stage & SHADER_STAGE_VERT_BIT) result |= VK_SHADER_STAGE_VERTEX_BIT;
    if (stage & SHADER_STAGE_FRAG_BIT) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (stage & SHADER_STAGE_COMP_BIT) result |= VK_SHADER_STAGE_COMPUTE_BIT;

    return result;
}
