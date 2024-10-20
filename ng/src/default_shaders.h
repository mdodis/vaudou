#ifndef VD_DEFAULT_SHADERS_H
#define VD_DEFAULT_SHADERS_H

const char *VD_GLSL = 
#include "shd/generated/vd.glsl"
;

const char *VD_PBROPAQUE_VERT =
#include "shd/generated/pbropaque.vert"
;

const char *VD_PBROPAQUE_FRAG =
#include "shd/generated/pbropaque.frag"
;

#endif // !VD_DEFAULT_SHADERS_H
