#include "shdc.h"
#include "glslang_c_interface.h"
#include "resource_limits_c.h"
#include <stdlib.h>

#include "array.h"

enum {
    MAX_INCLUDE_MAPPINGS = 10,
};

struct VD_SHDC {
    VD_SHDC_InitInfo info;

    u32                     num_include_mappings;
    glsl_include_result_t   include_mappings[MAX_INCLUDE_MAPPINGS];
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

    shdc->num_include_mappings = info->num_include_mappings;

    for (u32 i = 0; i < info->num_include_mappings; ++i) {
        shdc->include_mappings[i].header_name = info->include_mappings[i].file;
        shdc->include_mappings[i].header_data = info->include_mappings[i].code;
        shdc->include_mappings[i].header_length = strlen(info->include_mappings[i].code);
    }

    glslang_initialize_process();
}

static glsl_include_result_t null_result = { 0 };

/* Callback for local file inclusion */
static glsl_include_result_t* glsl_include_local(
    void* ctx,
    const char* header_name,
    const char* includer_name,
    size_t include_depth)
{
    return 0;
}

/* Callback for system file inclusion */
static glsl_include_result_t* glsl_include_system(
    void* ctx,
    const char* header_name,
    const char* includer_name,
    size_t include_depth)
{
    VD_SHDC *shdc = (VD_SHDC*)ctx;

    for (u32 i = 0; i < shdc->num_include_mappings; ++i) {
        if (strcmp(shdc->include_mappings[i].header_name, header_name) == 0) {
            return &shdc->include_mappings[i];
        }
    }

    return 0;
}

/* Callback for include result destruction */
static int glsl_free_include_result(void* ctx, glsl_include_result_t* result)
{
    return 0;
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
        .callbacks_ctx = (void*)shdc,
        .callbacks = {
            .include_local = glsl_include_local,
            .include_system = glsl_include_system,
            .free_include_result = glsl_free_include_result,
        },
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
