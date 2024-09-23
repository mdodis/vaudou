#include "builtin.h"
#include "mm.h"
#include "renderer.h"

/* ----WINDOWS----------------------------------------------------------------------------------- */
ECS_COMPONENT_DECLARE(Size2D);
ECS_COMPONENT_DECLARE(WindowComponent);
ECS_COMPONENT_DECLARE(WindowSurfaceComponent);
ECS_COMPONENT_DECLARE(Application);
ECS_DECLARE(AppQuitEvent);
ECS_OBSERVER_DECLARE(RendererOnWindowComponentSet);

/* ----MEMORY------------------------------------------------------------------------------------ */
ECS_COMPONENT_DECLARE(MemoryComponent);
ECS_SYSTEM_DECLARE(GarbageCollectTask);
ECS_SYSTEM_DECLARE(FreeFrameAllocationSystem);


void BuiltinImport(ecs_world_t *world)
{
	ECS_MODULE(world, Builtin);
	ECS_COMPONENT_DEFINE(world, Size2D);
	ECS_COMPONENT_DEFINE(world, WindowComponent);
	ECS_COMPONENT_DEFINE(world, WindowSurfaceComponent);
	ECS_COMPONENT_DEFINE(world, Application);
	AppQuitEvent = ecs_new(world);

	ECS_OBSERVER_DEFINE(world, RendererOnWindowComponentSet, EcsOnSet, WindowComponent);

	ECS_COMPONENT_DEFINE(world, MemoryComponent);
	ecs_set_hooks(world, MemoryComponent, {
		.dtor = MemoryComponentDtor,
	});

	ECS_SYSTEM_DEFINE(world, GarbageCollectTask, EcsPostFrame, 0);
	ECS_SYSTEM_DEFINE(world, FreeFrameAllocationSystem, EcsPostFrame, 0);
}