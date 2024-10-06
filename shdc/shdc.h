#ifndef VD_SHDC_H
#define VD_SHDC_H

#include "vd_common.h"

typedef struct VD_SHDC VD_SHDC;

typedef struct {
    void (*cb_error)(const char *what, const char *msg, const char *extmsg);
} VD_SHDC_InitInfo;

typedef enum {
    VD_SHDC_SHADER_STAGE_VERTEX,
    VD_SHDC_SHADER_STAGE_FRAGMENT,
    VD_SHDC_SHADER_STAGE_COMPUTE,
    VD_SHDC_SHADER_STAGE_GEOMETRY,
} VD_SHDC_ShaderStage;

VD_SHDC *vd_shdc_create();

void vd_shdc_init(VD_SHDC *shdc, VD_SHDC_InitInfo *info);
int vd_shdc_compile(
    VD_SHDC *shdc,
    const char *src,
    VD_SHDC_ShaderStage stage,
    VD_Allocator *alloc,
    void **result,
    size_t *result_size_bytes);
void vd_shdc_deinit(VD_SHDC *shdc);

#endif // !VD_SHDC_H