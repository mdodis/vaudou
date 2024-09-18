#ifndef VD_BUILTIN_H
#define VD_BUILTIN_H
#include "flecs.h"

/* ----WINDOWS----------------------------------------------------------------------------------- */
typedef struct {
	int x;
	int y;
} PixelSize;

typedef struct {
	int reserved;
} Window;

#define VD_WINDOW_CONTAINER_CREATE_WINDOW_PROC(name)		void *name(const char *title, void *usrdata)
#define VD_WINDOW_CONTAINER_DESTROY_WINDOW_PROC(name)		void name(void *window, void *usrdata)
#define VD_WINDOW_CONTAINER_POLL_PROC(name)					void name(void *usrdata)
typedef VD_WINDOW_CONTAINER_CREATE_WINDOW_PROC(VD_WindowContainerCreateWindowProc);
typedef VD_WINDOW_CONTAINER_DESTROY_WINDOW_PROC(VD_WindowContainerDestroyWindowProc);
typedef VD_WINDOW_CONTAINER_POLL_PROC(VD_WindowContainerPollProc);

typedef struct {
	VD_WindowContainerCreateWindowProc		*create_window;
	VD_WindowContainerDestroyWindowProc		*destroy_window;
	VD_WindowContainerPollProc				*poll;
	void									*usrdata;
} Application;

extern ECS_COMPONENT_DECLARE(Window);
extern ECS_COMPONENT_DECLARE(PixelSize);
extern ECS_COMPONENT_DECLARE(Application);
extern ECS_DECLARE(AppQuitEvent);

/* ----MEMORY------------------------------------------------------------------------------------ */
typedef struct MemoryManager MemoryManager;

extern ECS_COMPONENT_DECLARE(MemoryManager);
extern ECS_SYSTEM_DECLARE(GarbageCollectSystem);

void BuiltinImport(ecs_world_t *world);


#endif // !VD_BUILTIN_H