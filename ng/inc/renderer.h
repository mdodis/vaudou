#ifndef VD_RENDERER_H
#define VD_RENDERER_H

#include "volk.h"

#include "vd_common.h"
#include "array.h"
#include "builtin.h"

#include "r/types.h"
#include "r/deletion_queue.h"
#include "r/descriptor_allocator.h"

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
    VkCommandPool           command_pool;
    VkCommandBuffer         command_buffer;
    VkFence                 fnc_render_complete;
    VkSemaphore             sem_image_available;
    VkSemaphore             sem_present_image;
    VD_DescriptorAllocator  descriptor_allocator;
    VD_DeletionQueue        deletion_queue;
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
    VD_R_AllocatedImage             depth_image;
};

VD_Renderer *vd_renderer_create();
int vd_renderer_init(VD_Renderer *renderer, VD_RendererInitInfo *info);
int vd_renderer_deinit(VD_Renderer *renderer);

VkCommandBuffer vd_renderer_imm_begin(VD_Renderer *renderer);
void vd_renderer_imm_end(VD_Renderer *renderer);

VD_R_AllocatedBuffer vd_renderer_create_buffer(
    VD_Renderer *renderer,
    size_t size,
    VkBufferUsageFlags flags,
    VmaMemoryUsage usage);
void vd_renderer_destroy_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer);

void *vd_renderer_map_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer);
void vd_renderer_unmap_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer);

VkDevice vd_renderer_get_device(VD_Renderer *renderer);

HandleOf(VD_R_AllocatedImage) vd_renderer_create_texture(
    VD_Renderer *renderer,
    VD_R_TextureCreateInfo *info);

HandleOf(VD_R_GPUMesh) vd_renderer_create_mesh(
    VD_Renderer *renderer,
    VD_R_MeshCreateInfo *info);

void vd_renderer_write_mesh(
    VD_Renderer *renderer,
    VD_R_MeshWriteInfo *info);

HandleOf(GPUShader) vd_renderer_create_shader(
    VD_Renderer *renderer,
    GPUShaderCreateInfo *info);

HandleOf(GPUMaterialBlueprint) vd_renderer_create_material_blueprint(
    VD_Renderer *renderer,
    MaterialBlueprint *blueprint);

HandleOf(GPUMaterial) vd_renderer_create_material(
    VD_Renderer *renderer,
    HandleOf(GPUMaterialBlueprint) blueprint);

HandleOf(GPUMaterial) vd_renderer_get_default_material(VD_Renderer *renderer);

void vd_renderer_push_render_object(VD_Renderer *renderer, RenderObject *render_object);

void vd_renderer_upload_texture_data(
    VD_Renderer *renderer,
    VD_R_AllocatedImage *image,
    void *data,
    size_t size);

void vd_renderer_destroy_texture(
    VD_Renderer *renderer,
    VD_R_AllocatedImage *image);

// ----OBSERVERS------------------------------------------------------------------------------------
extern void RendererOnWindowComponentSet(ecs_iter_t *it);

// ----SYSTEMS--------------------------------------------------------------------------------------
extern void RendererRenderToWindowSurfaceComponents(ecs_iter_t *it);
extern void RendererCheckWindowComponentSizeChange(ecs_iter_t *it);
extern void RendererGatherStaticMeshComponentSystem(ecs_iter_t *it);
#endif // !VD_RENDERER_H
