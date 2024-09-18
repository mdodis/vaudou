#include "builtin.h"
#include "mm.h"

/* ----WINDOWS----------------------------------------------------------------------------------- */
ECS_COMPONENT_DECLARE(PixelSize);
ECS_COMPONENT_DECLARE(Window);
ECS_COMPONENT_DECLARE(Application);
ECS_DECLARE(AppQuitEvent);

/* ----MEMORY------------------------------------------------------------------------------------ */
ECS_COMPONENT_DECLARE(MemoryManager);
ECS_SYSTEM_DECLARE(GarbageCollectTask);


void BuiltinImport(ecs_world_t *world)
{
	ECS_MODULE(world, Builtin);
	ECS_COMPONENT_DEFINE(world, PixelSize);
	ECS_COMPONENT_DEFINE(world, Window);
	ECS_COMPONENT_DEFINE(world, Application);
	AppQuitEvent = ecs_new(world);

	//ECS_COMPONENT_DEFINE(world, MemoryManager);
	ECS_SYSTEM_DEFINE(world, GarbageCollectTask, EcsPostFrame, 0);
}