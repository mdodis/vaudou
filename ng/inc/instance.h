#ifndef VD_INSTANCE_H
#define VD_INSTANCE_H
#include "delegate.h"

VD_DELEGATE_DECLARE_PARAMS1_VOID(VD_UpdateDelegate, float, delta)

typedef VD_HOOK(VD_UpdateDelegate) VD_UpdateHook;

typedef struct VD_Instance VD_Instance;

typedef struct {
	struct {
		u32			num_enabled_extensions;
		const char **enabled_extensions;
	} vulkan;
} VD_InstanceInitInfo;

VD_Instance *vd_instance_create();
void vd_instance_init(VD_Instance *instance, VD_InstanceInitInfo *info);
void vd_instance_main(VD_Instance *instance);
void vd_instance_deinit(VD_Instance *instance);
void vd_instance_destroy(VD_Instance *instance);

VD_UpdateHook *vd_instance_get_update_hook(VD_Instance *instance);

struct ecs_world_t *vd_instance_get_world(VD_Instance *instance);

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