#include "instance.h"
#include "builtin.h"
#include "ed.h"
#include "sdl_window_container.h"

static struct {
    VD_Instance *instance;
} G;

int main(int argc, char const *argv[]) {
    G.instance = vd_instance_create();
    vd_instance_init(G.instance);
    ecs_world_t *world = vd_instance_get_world(G.instance);
    ecs_singleton_set(world, EcsRest, { 0 });
    ECS_IMPORT(world, Sdl);


    vd_instance_main(G.instance);

    vd_instance_deinit(G.instance);
    vd_instance_destroy(G.instance);
    return 0;
}
