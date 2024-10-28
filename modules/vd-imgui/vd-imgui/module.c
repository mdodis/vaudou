#include "vd-imgui/module.h"
#include "builtin.h"
#include "module_internal.h"
#include "r/types.h"
#include "instance.h"
#include "renderer.h"

typedef struct {
    VD_Renderer *renderer;
    HandleOf(VD_R_AllocatedImage) font_image;
    int a;
} BackendData;

static void ImGuiUpdatePanelsSystem(ecs_iter_t *it);

ECS_COMPONENT_DECLARE(ImGuiPanel);

void VdImGuiImport(ecs_world_t *world)
{
    ECS_MODULE(world, ImGui);

    init_imgui(& (InitInfo) {
        .backend_size = sizeof(BackendData),
        .backend_ptr = 0,
    });

    ECS_COMPONENT_DEFINE(world, ImGuiPanel);

    ecs_system(world,
    {
        .entity = ecs_entity(world,
        {
            .name = "ImGuiUpdatePanelsSystem",
        }),
        .query.terms = {
            (ecs_term_t) { .id = ecs_id(ImGuiPanel), .inout = EcsIn },
        },
        .callback = ImGuiUpdatePanelsSystem,
    });

    vd_instance_set(ecs_singleton_get(world, Application)->instance);

    VD_Renderer *renderer = vd_instance_get_renderer(vd_instance_get());

    BackendData *backend = get_backend_data();

    int width, height;
    void *font_data = get_fonts_texture_data(&width, &height);
    backend->font_image = vd_renderer_create_texture(renderer, & (VD_R_TextureCreateInfo) {
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
        .extent = { .width = width, .height = height, .depth = 1 },
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .mipmapping.on = 0,
    });

    vd_renderer_upload_texture_data(
        renderer,
        USE_HANDLE(backend->font_image, VD_R_AllocatedImage),
        font_data,
        width * height * sizeof(u32));
}

static void ImGuiUpdatePanelsSystem(ecs_iter_t *it)
{
    const ImGuiPanel *panels = ecs_field(it, ImGuiPanel, 0);

    for (int i = 0; i < it->count; ++i) {
        begin_frame(& (BeginFrameInfo) {
            .delta_time = it->delta_time,
            .w = 0,
            .h = 0,
            .rw = 0,
            .rh = 0,
        });

        end_frame();
    }

    render();
}
