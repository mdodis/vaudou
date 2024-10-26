#ifndef VD_R_SYS_SHADER_H
#define VD_R_SYS_SHADER_H
#include "handlemap.h"
#include "r/types.h"
#include "shdc.h"

typedef struct {
    VD_HANDLEMAP GPUShader  *shaders;
    VkDevice                device;
    VD_SHDC                 *compiler;
} SShader;

typedef struct {
    VkDevice device;
} SShaderInitInfo;

int vd_r_sshader_init(SShader *s, SShaderInitInfo *info);

HandleOf(GPUShader) vd_r_sshader_new(SShader *s, GPUShaderCreateInfo *info);

void vd_r_sshader_deinit(SShader *s);

#endif // !VD_R_SYS_SHADER_H
