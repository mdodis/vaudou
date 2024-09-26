#ifndef VD_RENDERER_H
#define VD_RENDERER_H

#include "volk.h"

#include "common.h"
#include "array.h"
#include "builtin.h"

#include "r/types.h"
#include "r/deletion_queue.h"

typedef struct VD_Instance  VD_Instance;
typedef struct VD_Renderer  VD_Renderer;
typedef struct ecs_iter_t   ecs_iter_t;
typedef struct ecs_world_t  ecs_world_t;

typedef struct {
    VD_Instance     *instance;
    ecs_world_t     *world;

    struct {
        u32                                         num_enabled_extensions;
        const char	                                **enabled_extensions;
        VD_GetPhysicalDevicePresentationSupportProc *get_physical_device_presentation_support;
        void                                        *usrdata;
    } vulkan;
} VD_RendererInitInfo;

typedef struct {
    VkCommandPool       command_pool;
    VkCommandBuffer     command_buffer;
    VkFence             fnc_render_complete;
    VkSemaphore         sem_image_available;
    VkSemaphore         sem_present_image;
    VD_DeletionQueue    deletion_queue;
} VD_RendererFrameData;

struct WindowSurfaceComponent {
    VkSwapchainKHR                  swapchain;
    VkSurfaceKHR                    surface;
    VkFormat                        surface_format;
    VD_ARRAY VkImage                *images;
    VD_ARRAY VkImageView            *image_views;
    VkExtent2D                      extent;
    VD_ARRAY VD_RendererFrameData   *frame_data;
    int                             current_frame;
    VD_R_AllocatedImage             color_image;
};

VD_Renderer *vd_renderer_create();
int vd_renderer_init(VD_Renderer *renderer, VD_RendererInitInfo *info);
int vd_renderer_deinit(VD_Renderer *renderer);

extern void RendererOnWindowComponentSet(ecs_iter_t *it);
extern void RendererRenderToWindowSurfaceComponents(ecs_iter_t *it);

#endif // !VD_RENDERER_H