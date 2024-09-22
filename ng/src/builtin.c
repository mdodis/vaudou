#include "builtin.h"
#include "mm.h"
#include "renderer.h"

/* ----WINDOWS----------------------------------------------------------------------------------- */
ECS_COMPONENT_DECLARE(PixelSize);
ECS_COMPONENT_DECLARE(WindowComponent);
ECS_COMPONENT_DECLARE(WindowSurfaceComponent);
ECS_COMPONENT_DECLARE(Application);
ECS_DECLARE(AppQuitEvent);
ECS_OBSERVER_DECLARE(RendererOnWindowComponentSet);

/* ----MEMORY------------------------------------------------------------------------------------ */
ECS_SYSTEM_DECLARE(GarbageCollectTask);


void BuiltinImport(ecs_world_t *world)
{
	ECS_MODULE(world, Builtin);
	ECS_COMPONENT_DEFINE(world, PixelSize);
	ecs_struct(world, {
		.entity = ecs_id(PixelSize),
		.members = {
			{.name = "x", .type = ecs_id(ecs_i32_t)},
			{.name = "y", .type = ecs_id(ecs_i32_t)},
		},
	});
	ECS_COMPONENT_DEFINE(world, WindowComponent);
	ECS_COMPONENT_DEFINE(world, WindowSurfaceComponent);
	ECS_COMPONENT_DEFINE(world, Application);
	AppQuitEvent = ecs_new(world);

	ECS_OBSERVER_DEFINE(world, RendererOnWindowComponentSet, EcsOnSet, WindowComponent);

	ECS_SYSTEM_DEFINE(world, GarbageCollectTask, EcsPostFrame, 0);
}