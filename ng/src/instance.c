#define VD_INTERNAL_SOURCE_FILE 1
#include "instance.h"

#include "flecs.h"

#include "subsystem.h"
#include "builtin.h"
#include "renderer.h"
#include "mm.h"
#include "sys.h"
#include "fmt.h"
#include "cvar.h"

#include <stdlib.h>

#define VD_LOG_IMPLEMENTATION
#include "vd_log.h"

#include "tracy/TracyC.h"

VD_Instance *Local_Instance;

struct VD_Instance {
    VD_Renderer			    *r;
    VD_SubsytemManager		sm;
    VD_UpdateHook			on_update;
    ecs_world_t				*world;
    VD_MM					*mm;
    VD_CVS                  *cvs;
    VD_Log					log;
    int should_close;
};


static void vd_ecs_log(int32_t level, const char *file, int32_t line, const char *msg);

VD_Instance* vd_instance_create()
{
    return calloc(1, sizeof(VD_Instance));
}

void OnApplicationQuit(ecs_iter_t *it)
{
    VD_Instance *instance = (VD_Instance *)it->callback_ctx;
    instance->should_close = 1;
}

void vd_instance_init(VD_Instance *instance, VD_InstanceInitInfo *info)
{
    TracyCZoneN(Initialize_All, "Initialize::All", 1);

// ----MM-------------------------------------------------------------------------------------------
    TracyCZoneN(Initialize_MM, "Initialize::MM", 1);
    
    Local_Instance = instance;
    instance->mm = vd_mm_create();
    vd_mm_init(instance->mm);

    TracyCZoneEnd(Initialize_MM);

// ----LOG------------------------------------------------------------------------------------------
    TracyCZoneN(Initialize_Log, "Initialize::Log", 1);

    str exec_path = vd_get_exec_path(vd_mm_get_frame_arena(instance->mm));
    str log_path = vd_snfmt(
            vd_mm_get_frame_arena(
                instance->mm),
                "%{stru32}/engine.vdlog%{null}",
                vd_str_chop_right_last_of(exec_path, '/'));

    instance->log.filepath = _strdup(log_path.data);
    instance->log.flags = VD_LOG_WRITE_STDOUT;
    vd_fmt_printf("%{stru32}\n", log_path);

    VD_LOG_SET(&instance->log);
    VD_LOG_RESET();

    VD_LOG("Instance", ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Begin Log");

    TracyCZoneEnd(Initialize_Log);
// ----CVS------------------------------------------------------------------------------------------
    TracyCZoneN(Initialize_CVS, "Initialize::CVS", 1);

    instance->cvs = vd_cvs_create();
    vd_cvs_init(instance->cvs);

    TracyCZoneEnd(Initialize_CVS);
// ----FLECS----------------------------------------------------------------------------------------
    TracyCZoneN(Initialize_Flecs, "Initialize::Flecs", 1);

    ecs_os_set_api_defaults();
    ecs_os_api_t os_api = ecs_os_get_api();
    os_api.log_ = vd_ecs_log;
    ecs_os_set_api(&os_api);
    instance->world = ecs_init();
    ecs_set_threads(instance->world, 4);

    ECS_IMPORT(instance->world, Builtin);

    ecs_singleton_set(instance->world, Application, { .instance = instance });

    TracyCZoneEnd(Initialize_Flecs);
// ----RENDERER--------------------------------------------------------------------------------------
    TracyCZoneN(Initialize_Renderer, "Initialize::Renderer", 1);

    instance->r = vd_renderer_create();
    vd_renderer_init(instance->r, &(VD_RendererInitInfo) {
        .instance   = instance,
        .world 	    = instance->world,
        .vulkan     = {
            .enabled_extensions                         = info->vulkan.enabled_extensions,
            .num_enabled_extensions                     = info->vulkan.num_enabled_extensions,
            .get_physical_device_presentation_support   = info->vulkan.get_physical_device_presentation_support,
            .usrdata                                    = info->vulkan.usrdata,
        },
    });

    TracyCZoneEnd(Initialize_Renderer);

    ecs_observer(instance->world, {
        .events = {AppQuitEvent},
        .query.terms = {{ ecs_id(Application) }},
        .callback = OnApplicationQuit,
        .callback_ctx = instance,
        });

    TracyCZoneEnd(Initialize_All);
}

void vd_instance_main(VD_Instance *instance)
{
    ecs_progress(instance->world, 0.0f);
    ecs_entity_t w = ecs_entity(instance->world, { .name = "Vaudou" });
    ecs_add(instance->world, w, WindowComponent);

    WindowComponent *window_component = (WindowComponent*)
        ecs_get(instance->world, w, WindowComponent);
    window_component->flags = WINDOW_FLAG_RESIZABLE;

    ecs_modified(instance->world, w, WindowComponent);

    while (!instance->should_close) {
        ecs_progress(instance->world, 0.0f);
    }
    
}

void vd_instance_deinit(VD_Instance *instance)
{
// ----RENDERER-------------------------------------------------------------------------------------
    vd_renderer_deinit(instance->r);
// ----FLECS----------------------------------------------------------------------------------------
    ecs_fini(instance->world);
// ----CVS------------------------------------------------------------------------------------------
    vd_cvs_deinit(instance->cvs);
// ----MM-------------------------------------------------------------------------------------------
    vd_mm_deinit(instance->mm);
// ----LOG------------------------------------------------------------------------------------------
    VD_LOG("Instance", "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< End Log");
}

void vd_instance_destroy(VD_Instance *instance)
{
    free(instance);
}

VD_SubsytemManager *vd_instance_get_subsystem_manager(VD_Instance *instance)
{
    return &instance->sm;
}

VD_UpdateHook *vd_instance_get_update_hook(VD_Instance *instance)
{
    return &instance->on_update;
}

ecs_world_t *vd_instance_get_world(VD_Instance *instance)
{
    return instance->world;
}

VD_MM *vd_instance_get_mm(VD_Instance *instance)
{
    return instance->mm;
}

VD_Renderer *vd_instance_get_renderer(VD_Instance *instance)
{
    return instance->r;
}

VD_Log *vd_instance_get_log(VD_Instance *instance)
{
    return &instance->log;
}

VD_CVS *vd_instance_get_cvs(VD_Instance *instance)
{
    return instance->cvs;
}

static void vd_ecs_log(int32_t level, const char *file, int32_t line, const char *msg)
{
    VD_LOG_FMT("Flecs", "%{i32} %{cstr}::%{i32}: %{cstr}", level, file, line, msg);
}
