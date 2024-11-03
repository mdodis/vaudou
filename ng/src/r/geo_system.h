#ifndef VD_R_GEO_SYSTEM_H
#define VD_R_GEO_SYSTEM_H
#include "r/types.h"
#include "handlemap.h"
#include "r/svma.h"

typedef struct {
    VD_HANDLEMAP VD_R_GPUMesh   *meshes;
    VkDevice                    device;
    SVMA                        *svma;
} VD_R_GeoSystem;

typedef struct {
    VkDevice        device;
    SVMA            *svma;
} VD_R_GeoSystemInitInfo;

int vd_r_geo_system_init(VD_R_GeoSystem *s, VD_R_GeoSystemInitInfo *info);

HandleOf(VD_R_GPUMesh) vd_r_geo_system_new(VD_R_GeoSystem *s, VD_R_MeshCreateInfo *info);

int sgeo_resize(VD_R_GeoSystem *s, HandleOf(VD_R_GPUMesh) mesh, VD_R_MeshCreateInfo *info);

void vd_r_geo_system_deinit(VD_R_GeoSystem *s);

#endif // !VD_R_GEO_SYSTEM_H
