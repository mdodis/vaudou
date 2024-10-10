#ifndef VD_BUILTIN_H
#define VD_BUILTIN_H
#include "vd_common.h"

#include "delegate.h"

#include "flecs.h"
#include "cglm/cglm.h"

VD_DELEGATE_DECLARE_PARAMS1_VOID(VD_ImmediateDestroyEntity, ecs_entity_t, entity)

// ----GENERAL--------------------------------------------------------------------------------------
#define VD_DTOR_PROC(name) void name(void *ptr, i32 count, const ecs_type_info_t *type)

// ----WINDOWS--------------------------------------------------------------------------------------
typedef struct WindowComponent  WindowComponent;
typedef struct VD_Instance      VD_Instance;

#define VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(name) \
	int name(void *vk_instance, void *vk_physical_device, u32 queue_family_index, void *usrdata)
typedef VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(VD_GetPhysicalDevicePresentationSupportProc);

#define VD_WINDOW_CREATE_SURFACE_PROC(name) void *name(WindowComponent *window, void *instance)
typedef VD_WINDOW_CREATE_SURFACE_PROC(VD_WindowCreateSurfaceProc);

typedef struct {
    i32 x;
    i32 y;
} Size2D;

typedef enum {
    WINDOW_FLAG_NONE = 0,
    WINDOW_FLAG_RESIZABLE = 1 << 0,
} WindowFlags;

struct WindowComponent {
    VD_WindowCreateSurfaceProc              *create_surface;
    void                                    *window_ptr;
    VD_CALLBACK(VD_ImmediateDestroyEntity)  on_immediate_destroy;
    WindowFlags                             flags;
    int                                     just_resized;
};

/**
 * @brief Represents a window surface on the GPU.
 * @see renderer.h
 */
typedef struct WindowSurfaceComponent WindowSurfaceComponent;

typedef struct {
    VD_Instance *instance;
} Application;

extern ECS_COMPONENT_DECLARE(WindowComponent);
extern ECS_COMPONENT_DECLARE(WindowSurfaceComponent);
extern ECS_COMPONENT_DECLARE(Size2D);
extern ECS_COMPONENT_DECLARE(Application);
extern ECS_DECLARE(AppQuitEvent);

// ----MEMORY---------------------------------------------------------------------------------------

/**
 * @brief Used to store memory information for automatic deletion.
 * @note  Entity-local memory allocations last up until and including the entity's destruction.
 * @note  This is because the garbage collection task is run after the frame is finished.
 */
typedef struct {
    void *opaque_info_ptr;
} MemoryComponent;

extern ECS_COMPONENT_DECLARE(MemoryComponent);
extern VD_DTOR_PROC(MemoryComponentDtor);

extern ECS_SYSTEM_DECLARE(FreeFrameAllocationSystem);
extern ECS_SYSTEM_DECLARE(GarbageCollectSystem);

// ----RENDERER-------------------------------------------------------------------------------------
extern ECS_SYSTEM_DECLARE(RendererRenderToWindowSurfaceComponents); // EcsOnStore
extern ECS_SYSTEM_DECLARE(RendererCheckWindowComponentSizeChange);  // EcsOnPostLoad
extern ECS_OBSERVER_DECLARE(RendererOnWindowComponentSet);


void BuiltinImport(ecs_world_t *world);


#endif // !VD_BUILTIN_H