#define VD_INTERNAL_SOURCE_FILE 1
#include "sshader.h"
#include "vd_log.h"
#include "mm.h"
#include "vd_vk.h"
#include "vulkan_helpers.h"
#include "instance.h"

const char *GLSL_PREINCLUDE = 
#include "shd/generated/vd.glsl"
;

const char *GLSL_VD_STRUCTS = 
#include "shd/generated/vd.structs.glsl"
;

static void free_shader(void *object, void *c);
static void vd_shdc_log_error(const char *what, const char *msg, const char *extmsg);

int vd_r_sshader_init(SShader *s, SShaderInitInfo *info)
{
    s->device = info->device;
    VD_HANDLEMAP_INIT(s->shaders, {
        .allocator = vd_memory_get_system_allocator(),
        .initial_capacity = 64,
        .c = s,
        .on_free_object = free_shader,
    });

    s->compiler = vd_shdc_create();
    vd_shdc_init(s->compiler, &(VD_SHDC_InitInfo) {
        .cb_error = vd_shdc_log_error,
        .num_include_mappings = 2,
        .include_mappings = (VD_SHDC_IncludeMapping[]) {
            (VD_SHDC_IncludeMapping) { .file = "vd.glsl", .code = GLSL_PREINCLUDE },
            (VD_SHDC_IncludeMapping) { .file = "vd.structs.glsl", .code = GLSL_VD_STRUCTS },
        },
    });
    return 0;
}

HandleOf(GPUShader) vd_r_sshader_new(SShader *s, GPUShaderCreateInfo *info)
{
    GPUShader result;

    void *bytecode = info->bytecode;
    size_t bytecode_size = info->bytecode_size;

    if (info->sourcecode != 0 && info->sourcecode_len != 0) {
        VD_SHDC_ShaderStage shader_stage;
        switch (info->stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:   shader_stage = VD_SHDC_SHADER_STAGE_VERTEX;   break;
            case VK_SHADER_STAGE_GEOMETRY_BIT: shader_stage = VD_SHDC_SHADER_STAGE_GEOMETRY; break;
            case VK_SHADER_STAGE_FRAGMENT_BIT: shader_stage = VD_SHDC_SHADER_STAGE_FRAGMENT; break;
            default: exit(1);
        }

        int result = vd_shdc_compile(
            s->compiler,
            info->sourcecode,
            shader_stage,
            VD_MM_FRAME_ALLOCATOR(),
            &bytecode,
            &bytecode_size);
        if (result != 0) {
            return INVALID_HANDLE();
        }
    }

    result.stage = info->stage;
    VD_VK_CHECK(vd_vk_create_shader_module(s->device, bytecode, bytecode_size, &result.module));

    return VD_HANDLEMAP_REGISTER(s->shaders, &result, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT,
    });
}

void vd_r_sshader_deinit(SShader *s)
{
    VD_HANDLEMAP_DEINIT(s->shaders);
    vd_shdc_deinit(s->compiler);
}

static void free_shader(void *object, void *c)
{
    GPUShader *shader = (GPUShader*)object;
    SShader *s = (SShader*)c;
    vkDestroyShaderModule(s->device, shader->module, 0);
    VD_LOG("SShader", "Deleting Shader");
}

static void vd_shdc_log_error(const char *what, const char *msg, const char *extmsg)
{
    VD_LOG_FMT("SHDC", "%{cstr}: %{cstr} %{cstr}", what, msg, extmsg);
}
