#ifndef VD_INSTANCE_H
#define VD_INSTANCE_H
#include "delegate.h"
#include "builtin.h"
#include "vd_log.h"

VD_DELEGATE_DECLARE_PARAMS1_VOID(VD_UpdateDelegate, float, delta)

typedef VD_HOOK(VD_UpdateDelegate) VD_UpdateHook;

typedef struct VD_Instance 	VD_Instance;
typedef struct VD_MM        VD_MM;
typedef struct ecs_world_t 	ecs_world_t;
typedef struct VD_Renderer  VD_Renderer;

typedef struct {
    struct {
        u32											num_enabled_extensions;
        const char 									**enabled_extensions;
        VD_GetPhysicalDevicePresentationSupportProc *get_physical_device_presentation_support;
        void 										*usrdata;
    } vulkan;
} VD_InstanceInitInfo;

VD_Instance *vd_instance_create();
void vd_instance_init(VD_Instance *instance, VD_InstanceInitInfo *info);
void vd_instance_main(VD_Instance *instance);
void vd_instance_deinit(VD_Instance *instance);
void vd_instance_destroy(VD_Instance *instance);

VD_UpdateHook *vd_instance_get_update_hook(VD_Instance *instance);

ecs_world_t *vd_instance_get_world(VD_Instance *instance);
VD_MM 		*vd_instance_get_mm(VD_Instance *instance);
VD_Renderer *vd_instance_get_renderer(VD_Instance *instance);
VD_Log 		*vd_instance_get_log(VD_Instance *instance);

extern VD_Instance *Local_Instance;

static inline VD_Instance *vd_instance_get()
{
    return Local_Instance;
}

static inline VD_Instance *vd_instance_set(VD_Instance *instance)
{
    VD_Instance *prev = Local_Instance;
    Local_Instance = instance;
    return prev;
}


#endif // !VD_INSTANCE_H