#include "shdc.h"
#include "glslang_c_interface.h"
#include "resource_limits_c.h"
#include <stdlib.h>

struct VD_SHDC {
    VD_SHDC_InitInfo info;
};

static glslang_stage_t shdc_stage_to_glslang_stage(VD_SHDC_ShaderStage s)
{
    switch (s)
    {
        case VD_SHDC_SHADER_STAGE_VERTEX: return GLSLANG_STAGE_VERTEX;
        case VD_SHDC_SHADER_STAGE_FRAGMENT: return GLSLANG_STAGE_FRAGMENT;
        case VD_SHDC_SHADER_STAGE_COMPUTE: return GLSLANG_STAGE_COMPUTE;
        case VD_SHDC_SHADER_STAGE_GEOMETRY: return GLSLANG_STAGE_GEOMETRY;
        default: return GLSLANG_STAGE_VERTEX;
    }
}

VD_SHDC *vd_shdc_create()
{
    return calloc(1, sizeof(VD_SHDC));
}

void vd_shdc_init(VD_SHDC *shdc, VD_SHDC_InitInfo *info)
{
    shdc->info = *info;
    glslang_initialize_process();
}

int vd_shdc_compile(
    VD_SHDC *shdc,
    const char *src,
    VD_SHDC_ShaderStage stage,
    VD_Allocator *alloc,
    void **result,
    size_t *result_size_bytes)
{
    glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = shdc_stage_to_glslang_stage(stage),
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = src,
        .default_version = 450,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = 0,
        .forward_compatible = 0,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource(),
    };
    glslang_shader_t *shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        shdc->info.cb_error(
            "Preprocess Failure",
            glslang_shader_get_info_log(shader),
            glslang_shader_get_info_debug_log(shader));
        return -1;
    }

    if (!glslang_shader_parse(shader, &input)) {
        shdc->info.cb_error(
            "Parse Failure",
            glslang_shader_get_info_log(shader),
            glslang_shader_get_info_debug_log(shader));
        return -1;
    }

    glslang_program_t *program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        shdc->info.cb_error(
            "Link Failure",
            glslang_program_get_info_log(program),
            glslang_program_get_info_debug_log(program));
        return -1;
    }

    glslang_program_SPIRV_generate(program, shdc_stage_to_glslang_stage(stage));

    *result_size_bytes = glslang_program_SPIRV_get_size(program) * sizeof(u32);
    *result = (void*)alloc->proc_alloc(
        0,
        0,
        *result_size_bytes,
        alloc->c);

    glslang_program_SPIRV_get(program, (u32*)*result);

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return 0;
}

void vd_shdc_deinit(VD_SHDC *shdc)
{
    glslang_finalize_process();
}