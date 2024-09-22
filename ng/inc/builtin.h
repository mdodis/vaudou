#ifndef VD_BUILTIN_H
#define VD_BUILTIN_H
#include "common.h"

#include "flecs.h"

/* ----WINDOWS----------------------------------------------------------------------------------- */
typedef struct {
    int x;
    int y;
} PixelSize;

typedef struct WindowComponent  WindowComponent;
typedef struct VD_Instance      VD_Instance;


#define VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(name) \
	int name(void *vk_instance, void *vk_physical_device, u32 queue_family_index, void *usrdata)
typedef VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(VD_GetPhysicalDevicePresentationSupportProc);

#define VD_WINDOW_CREATE_SURFACE_PROC(name) void *name(WindowComponent *window, void *instance)
typedef VD_WINDOW_CREATE_SURFACE_PROC(VD_WindowCreateSurfaceProc);

struct WindowComponent {
    VD_WindowCreateSurfaceProc  *create_surface;
    void                        *window_ptr;
};

/**
 * @brief Represents a window surface on the GPU.
 * @see renderer.h
 */
typedef struct WindowSurfaceComponent WindowSurfaceComponent;

#define VD_WINDOW_CONTAINER_CREATE_WINDOW_PROC(name)	void *name(const char *title, void *usrdata)
#define VD_WINDOW_CONTAINER_DESTROY_WINDOW_PROC(name)	void name(void *window, void *usrdata)
#define VD_WINDOW_CONTAINER_POLL_PROC(name)				void name(void *usrdata)
#define VD_WINDOW_CONTAINER_CREATE_WINDOW_SURFACE_PROC(name) \
    void *name(void *window, void *instance, void *usrdata)
typedef VD_WINDOW_CONTAINER_CREATE_WINDOW_PROC(VD_WindowContainerCreateWindowProc);
typedef VD_WINDOW_CONTAINER_DESTROY_WINDOW_PROC(VD_WindowContainerDestroyWindowProc);
typedef VD_WINDOW_CONTAINER_POLL_PROC(VD_WindowContainerPollProc);
typedef VD_WINDOW_CONTAINER_CREATE_WINDOW_SURFACE_PROC(VD_WindowContainerCreateWindowSurfaceProc);

typedef struct {
    VD_Instance *instance;
} Application;

extern ECS_COMPONENT_DECLARE(WindowComponent);
extern ECS_COMPONENT_DECLARE(WindowSurfaceComponent);
extern ECS_COMPONENT_DECLARE(PixelSize);
extern ECS_COMPONENT_DECLARE(Application);
extern ECS_DECLARE(AppQuitEvent);
extern ECS_OBSERVER_DECLARE(RendererOnWindowComponentSet);

/* ----MEMORY------------------------------------------------------------------------------------ */

extern ECS_SYSTEM_DECLARE(GarbageCollectSystem);

void BuiltinImport(ecs_world_t *world);


#endif // !VD_BUILTIN_H