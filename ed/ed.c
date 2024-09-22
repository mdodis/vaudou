#include "instance.h"
#include "builtin.h"
#include "ed.h"
#include "sdl_window_container.h"

static struct {
    VD_Instance *instance;
} G;

int main(int argc, char const *argv[]) {
    G.instance = vd_instance_create();

    u32 num_extensions;
    const char **extensions;
    get_required_extensions(&num_extensions, &extensions);
    vd_instance_init(G.instance, &(VD_InstanceInitInfo) {
        .vulkan = {
            .num_enabled_extensions                     = num_extensions,
            .enabled_extensions                         = extensions,
            .get_physical_device_presentation_support   = sdl_get_physical_device_presentation_support_proc,
        },
    });

    ecs_world_t *world = vd_instance_get_world(G.instance);
    ecs_singleton_set(world, EcsRest, { 0 });
    ECS_IMPORT(world, Sdl);


    vd_instance_main(G.instance);

    vd_instance_deinit(G.instance);
    vd_instance_destroy(G.instance);
    return 0;
}
