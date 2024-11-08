#define VD_INTERNAL_SOURCE_FILE 1
#define VD_LOG_IMPLEMENTATION
#include "instance.h"
#include "mm.h"
#include "array.h"
#include "builtin.h"
#include "ed.h"
#include "vd_log.h"
#include "w_sdl.h"
#include "vd-imgui/module.h"
#include "renderer.h"

static struct {
    VD_Instance *instance;
} G;

int main(int argc, char const *argv[]) {
    G.instance = vd_instance_create();

    u32 num_extensions;
    const char **extensions;
    vd_w_sdl_get_required_extensions(&num_extensions, &extensions);
    vd_instance_init(G.instance, &(VD_InstanceInitInfo) {
        .vulkan = {
            .num_enabled_extensions                     = num_extensions,
            .enabled_extensions                         = extensions,
            .get_physical_device_presentation_support   = 
                vd_w_sdl_get_physical_device_presentation_support_proc,
        },
    });

    VD_LOG_SET(vd_instance_get_log(G.instance));



    ecs_world_t *world = vd_instance_get_world(G.instance);
    ecs_singleton_set(world, EcsRest, { 0 });
    ECS_IMPORT(world, Sdl);
    ECS_IMPORT(world, VdImGui);

    {
        ecs_entity_t w = ecs_entity(world, { .name = "Vaudou" });
        ecs_add(world, w, WindowComponent);

        WindowComponent *window_component = (WindowComponent*)
            ecs_get(world, w, WindowComponent);
        window_component->flags = WINDOW_FLAG_RESIZABLE;

        ecs_modified(world, w, WindowComponent);
    }

    VD_Renderer *renderer = vd_instance_get_renderer(G.instance);
    HandleOf(GPUMaterialBlueprint) pbropaque = vd_renderer_get_default_handle(
        renderer,
        RENDERER_DEFAULT_MATERIAL_PBROPAQUE);

    dynarray PtrTo(ecs_entity_t) windows = 0;
    array_init(windows, VD_MM_FRAME_ALLOCATOR());
    all_windows(G.instance, &windows);

    ecs_entity_t first_window = windows[0];

    {
        ecs_entity_t ent = ecs_entity(world, { .name = "sphere" });
        ecs_set(world, ent, StaticMeshComponent, {
            .mesh = vd_renderer_get_default_handle(renderer, RENDERER_DEFAULT_MESH_SPHERE),
            .material = vd_renderer_create_material(renderer, pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        glm_translate_x(matrix, -1.0f);

        ecs_add(world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

        ecs_add_pair(world, ent, EcsChildOf, first_window);

    }

    {
        ecs_entity_t ent = ecs_entity(world, { .name = "cube" });
        ecs_set(world, ent, StaticMeshComponent, {
            .mesh = vd_renderer_get_default_handle(renderer, RENDERER_DEFAULT_MESH_CUBE),
            .material = vd_renderer_create_material(renderer, pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, 1.0f);
        glm_rotate(matrix, glm_rad(45.0f), rotation_vector);

        ecs_add(world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);
        ecs_add_pair(world, ent, EcsChildOf, first_window);

    }

    {
        ecs_entity_t ent = ecs_entity(world, { .name = "cube 1" });
        ecs_set(world, ent, StaticMeshComponent, {
            .mesh = vd_renderer_get_default_handle(renderer, RENDERER_DEFAULT_MESH_CUBE),
            .material = vd_renderer_create_material(renderer, pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, -1.1f);
        glm_rotate(matrix, glm_rad(-45.0f), rotation_vector);

        ecs_add(world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

        ecs_add_pair(world, ent, EcsChildOf, first_window);
    }
    {
        ecs_entity_t ent = ecs_entity(world, { .name = "cube 2" });
        HandleOf(GPUMaterial) mat = vd_renderer_create_material(
            renderer,
            pbropaque);

        USE_HANDLE(mat, GPUMaterial)->properties[0].sampler2d = vd_renderer_get_default_handle(
            renderer,
            RENDERER_DEFAULT_TEXTURE_WHITE);
        ecs_set(world, ent, StaticMeshComponent, {
            .mesh = vd_renderer_get_default_handle(renderer, RENDERER_DEFAULT_MESH_CUBE),
            .material = mat,
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, -1.1f);
        glm_translate_x(matrix, -1);
        glm_rotate(matrix, glm_rad(25.0f), rotation_vector);

        ecs_add(world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

        ecs_add_pair(world, ent, EcsChildOf, first_window);
    }

    vd_instance_main(G.instance);

    vd_instance_deinit(G.instance);
    vd_w_sdl_deinit();
    vd_instance_destroy(G.instance);
    return 0;
}
