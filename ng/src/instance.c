#define VD_INTERNAL_SOURCE_FILE 1
#include "instance.h"
#include "stdlib.h"
#include "subsystem.h"
#include "flecs.h"
#include "builtin.h"
#include "renderer.h"

VD_Instance *Local_Instance;

struct VD_Instance {
	VD_Renderer			    *r;
	VD_SubsytemManager		sm;
	VD_UpdateHook			on_update;
	ecs_world_t				*world;
	int should_close;
};

VD_Instance* vd_instance_create()
{
	return calloc(1, sizeof(VD_Instance));
}

void OnApplicationQuit(ecs_iter_t *it)
{
	VD_Instance *instance = (VD_Instance *)it->callback_ctx;
	instance->should_close = 1;
}

void vd_instance_init(VD_Instance *instance, VD_InstanceInitInfo *info)
{
	Local_Instance = instance;
	instance->r = vd_renderer_create();
	vd_renderer_init(instance->r, &(VD_RendererInitInfo) {
		.instance = instance,
		.vulkan = {
			.enabled_extensions = info->vulkan.enabled_extensions,
			.num_enabled_extensions = info->vulkan.num_enabled_extensions,
		},
	});

	instance->world = ecs_init();
	ECS_IMPORT(instance->world, Builtin);

	ecs_singleton_set(instance->world, Application, {});

	ecs_observer(instance->world, {
		.events = {AppQuitEvent},
		.query.terms = {{ ecs_id(Application) }},
		.callback = OnApplicationQuit,
		.callback_ctx = instance,
		});
}

void vd_instance_main(VD_Instance *instance)
{
	ecs_progress(instance->world, 0.0f);
	ecs_entity_t w = ecs_entity(instance->world, { .name = "Vaudou" });
	ecs_add(instance->world, w, Window);

	while(!instance->should_close) {
		ecs_progress(instance->world, 0.0f);
	}

	printf("Closing...\n");
}

void vd_instance_deinit(VD_Instance *instance)
{
	ecs_fini(instance->world);
	vd_renderer_deinit(instance->r);
}

void vd_instance_destroy(VD_Instance *instance)
{
	free(instance);
}

VD_SubsytemManager *vd_instance_get_subsystem_manager(VD_Instance *instance)
{
	return &instance->sm;
}

VD_UpdateHook *vd_instance_get_update_hook(VD_Instance *instance)
{
	return &instance->on_update;
}

ecs_world_t *vd_instance_get_world(VD_Instance *instance)
{
	return instance->world;
}


