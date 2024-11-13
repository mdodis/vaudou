#define VD_INTERNAL_SOURCE_FILE 1
#include "vd-imgui/module.h"
#include "builtin.h"
#include "module_internal.h"
#include "r/types.h"
#include "instance.h"
#include "renderer.h"
#include "mm.h"

const char *GUI_VERT_SHADER_SOURCE =
#include "shd/generated/gui.vert"
;

const char *GUI_FRAG_SHADER_SOURCE =
#include "shd/generated/gui.frag"
;

typedef struct {
    VD_Renderer *renderer;
    HandleOf(Texture) font_image;
    HandleOf(VD_R_GPUMesh) mesh_data;
    HandleOf(GPUMaterialBlueprint) blueprint;
    HandleOf(GPUMaterial) material;
} BackendData;

typedef struct {
    VkDeviceAddress vertex_buffer;
    vec2 scale;
    vec2 translate;
} GuiPushConstant;

static void ImGuiUpdatePanelsSystem(ecs_iter_t *it);
static void CollectMeshesSystem(ecs_iter_t *it);

ECS_COMPONENT_DECLARE(ImGuiPanel);

void VdImGuiImport(ecs_world_t *world)
{
    ECS_MODULE(world, VdImGui);

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
            .add = ecs_ids( ecs_dependson(EcsPostUpdate) ),
        }),
        .query.terms = {
            (ecs_term_t) { .id = ecs_id(ImGuiPanel), .inout = EcsIn },
        },
        .callback = ImGuiUpdatePanelsSystem,
    });

    ecs_system(world,
    {
        .entity = ecs_entity(world,
        {
            .name = "Collect Meshes System",
            .add = ecs_ids( ecs_dependson(EcsPreStore) ),
        }),
        .query.terms = {
            { .id = ecs_id(WindowSurfaceComponent), .inout = EcsIn },
        },
        .callback = CollectMeshesSystem,
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
        USE_HANDLE(backend->font_image, Texture),
        font_data,
        width * height * sizeof(u32));

    backend->mesh_data = vd_renderer_create_mesh(renderer, & (VD_R_MeshCreateInfo) {
        .num_indices = 8,
        .num_vertices = 8,
    });

    HandleOf(GPUShader) vert = vd_renderer_create_shader(renderer, & (GPUShaderCreateInfo) {
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .sourcecode = GUI_VERT_SHADER_SOURCE,
        .sourcecode_len = strlen(GUI_VERT_SHADER_SOURCE),
    });

    HandleOf(GPUShader) frag = vd_renderer_create_shader(renderer, & (GPUShaderCreateInfo) {
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .sourcecode = GUI_FRAG_SHADER_SOURCE,
        .sourcecode_len = strlen(GUI_FRAG_SHADER_SOURCE),
    });

    backend->blueprint = vd_renderer_create_material_blueprint(renderer, & (MaterialBlueprint) {
        .pass = 0,
        .num_shaders = 2,
        .shaders =
        {
            vert,
            frag,
        },
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .polygon_mode = VK_POLYGON_MODE_FILL,
        .num_properties = 1,
        .properties = (MaterialProperty[]) 
        {
            (MaterialProperty)
            {
                .binding.type = BINDING_TYPE_SAMPLER2D,
            }
        },
        .push_constant = {
            .info.type = PUSH_CONSTANT_TYPE_CUSTOM,
            .info.stage = SHADER_STAGE_VERT_BIT,
            .info.size = sizeof(GuiPushConstant),
        },
    });

    backend->material = vd_renderer_create_material(renderer, backend->blueprint);

    USE_HANDLE(backend->material, GPUMaterial)->properties[0].sampler2d = backend->font_image;

    ecs_entity_t panel_entity = ecs_entity(world, { .name = "imgui panel 1" });
    ecs_add(world, panel_entity, ImGuiPanel);
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

        imgui_text("hello!");

        end_frame();
    }

    render();
}

static void CollectMeshesSystem(ecs_iter_t *it)
{
    DrawList draw_list = get_draw_list();
    WindowSurfaceComponent *ws = ecs_field(it, WindowSurfaceComponent, 0);

    if (draw_list == 0) {
        return;
    }

    u32 *indices = 0;
    VD_R_Vertex *vertices = 0;
    size_t indices_write_offset = 0;
    size_t vertices_write_offset = 0;

    array_init(indices, VD_MM_FRAME_ALLOCATOR());
    array_init(vertices, VD_MM_FRAME_ALLOCATOR());

    array_addn(indices, draw_list_indx_count(draw_list));
    array_addn(vertices, draw_list_vert_count(draw_list));

    for (size_t i = 0; i < draw_list_cmdlist_count(draw_list); ++i) {
        CmdList cmd_list = draw_list_get_cmdlist(draw_list, i);

        u16 *indx_ptr = cmd_list_indx_ptr(cmd_list);
        for (size_t j = 0; j < cmd_list_indx_count(cmd_list); ++j) {
            indices[indices_write_offset + j] = indx_ptr[j];
        }
        indices_write_offset += cmd_list_indx_count(cmd_list);

        for (size_t j = 0; j < cmd_list_vert_count(cmd_list); ++j) {
            VD_R_Vertex v;
            cmd_list_vert_col(cmd_list, j, v.color);
            cmd_list_vert_pos(cmd_list, j, v.position);
            v.position[2] = 0.0f;
            vec2 uv;
            cmd_list_vert_tex(cmd_list, j, uv);
            v.uv_x = uv[0];
            v.uv_y = uv[1];
            vertices[vertices_write_offset + j] = v;
        }
        vertices_write_offset += cmd_list_indx_count(cmd_list);
    }

    VD_Renderer *renderer = vd_instance_get_renderer(vd_instance_get());
    BackendData *backend = get_backend_data();
    vd_renderer_write_mesh(renderer, & (VD_R_MeshWriteInfo) {
        .mesh = backend->mesh_data,
        .num_indices = array_len(indices),
        .num_vertices = array_len(vertices),
        .vertices = vertices,
        .indices = indices,
    });
    
    /*for (size_t i = 0; i < draw_list_cmdlist_count(draw_list); ++i) {*/
    /*    CmdList cmd_list = draw_list_get_cmdlist(draw_list, i);*/
    /**/
    /*    size_t index_count_offset = 0;*/
    /*    for (size_t j = 0; j < cmd_list_buf_count(cmd_list); ++j) {*/
    /*        CmdBuffer cmd_buffer = cmd_list_buf(cmd_list, j);*/
    /**/
    /*        unsigned int index_count = cmd_buffer_idx_count(cmd_buffer);*/
    /*        unsigned int index_offset = cmd_buffer_idx_offset(cmd_buffer);*/
    /**/
    /*        vec4 clip;*/
    /*        cmd_buffer_clip(cmd_buffer, clip);*/
    /**/
    /*        GuiPushConstant *gui_push_constant = VD_MM_FRAME_ALLOC_STRUCT(GuiPushConstant);*/
    /*        gui_push_constant->vertex_buffer = USE_HANDLE(*/
    /*            backend->mesh_data,*/
    /*            VD_R_GPUMesh)->vertex_buffer_address;*/
    /**/
    /*        vd_renderer_push_render_object(*/
    /*            renderer,*/
    /*            ws,*/
    /*            & (RenderObject)*/
    /*            {*/
    /*                .mesh = backend->mesh_data,*/
    /*                .material = backend->material,*/
    /*                .first_index = index_offset,*/
    /*                .index_count = index_count,*/
    /*                .push_constant.info = {*/
    /*                    .size = sizeof(GuiPushConstant),*/
    /*                    .type = PUSH_CONSTANT_TYPE_CUSTOM,*/
    /*                    .stage = SHADER_STAGE_VERT_BIT,*/
    /*                },*/
    /*                .push_constant.ptr = gui_push_constant,*/
    /*            });*/
    /**/
    /*    }*/
    /*    index_count_offset += cmd_list_indx_count(cmd_list);*/
    /*}*/

}
