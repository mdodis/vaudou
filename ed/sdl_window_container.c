#include "sdl_window_container.h"
#include <assert.h>
#include <stdlib.h>
#include "common.h"
#include "volk.h"
#include "SDL3/SDL_vulkan.h"
#include "vd_log.h"

static int Sdl_Initialized = 0;
static void ensure_sdl_initialized()
{
    if (!Sdl_Initialized) {
        assert(SDL_Init(SDL_INIT_VIDEO) == SDL_TRUE);
        assert(SDL_Vulkan_LoadLibrary(0) == SDL_TRUE);
        Sdl_Initialized = 1;
    }
}

static VD_WINDOW_CREATE_SURFACE_PROC(sdl_create_surface_proc) {
    ensure_sdl_initialized();
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface((SDL_Window *)window->window_ptr, instance, 0, &surface) != SDL_TRUE)
    {
        const char *error = SDL_GetError();
        printf("SDL Error: %s\n", error);
        abort();
    }
    return surface;
}

VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(sdl_get_physical_device_presentation_support_proc)
{
    ensure_sdl_initialized();
    SDL_bool result = SDL_Vulkan_GetPresentationSupport(
        *((VkInstance *)vk_instance),
        *((VkPhysicalDevice *)vk_physical_device),
        queue_family_index);

    if (!result) {
        const char *error = SDL_GetError();
        if (*error) {
            printf("SDL Error: %s\n", error);
            abort();
        }
    }

    return result == SDL_TRUE;
}

static void OnAddWindowObserver(ecs_iter_t *it)
{
    ensure_sdl_initialized();
    for (int i = 0; i < it->count; ++i) {
        const char *name = ecs_get_name(it->world, it->entities[i]);

        SDL_Window *window_ptr = SDL_CreateWindow(name, 640, 480, SDL_WINDOW_VULKAN);
        
        ecs_set(it->world, it->entities[i], Size2D, {
            .x = 640,
            .y = 480,
        });
        ecs_set(it->world, it->entities[i], WindowComponent, {
            .window_ptr = window_ptr,
            .create_surface = sdl_create_surface_proc,
        });
    }
}

static void OnSetWindowNameObserver(ecs_iter_t *it)
{
    ensure_sdl_initialized();
    WindowComponent *w = ecs_field(it, WindowComponent, 1);
    SDL_Window *window_ptr = (SDL_Window*)w->window_ptr;

    for (int i = 0; i < it->count; ++i) {
        const char *name = ecs_get_name(it->world, it->entities[i]);
        SDL_SetWindowTitle(window_ptr, name);
    }
}

static void PollSDLEvents(ecs_iter_t *it)
{
    ensure_sdl_initialized();
    const Application *app = ecs_singleton_get(it->world, Application);

    SDL_Event evt;
    while (SDL_PollEvent(&evt))
    {
        switch (evt.type)
        {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            {
                SDL_Window *closed_window = SDL_GetWindowFromID(evt.window.windowID);

                ecs_query_t *q = ecs_query(it->world, {
                    .terms = {
                        {.id = ecs_id(WindowComponent), },
                    }
                });

                ecs_iter_t qit = ecs_query_iter(it->world, q);
                int remaining_windows = qit.count;
                while (ecs_query_next(&qit)) {
                    WindowComponent *w = ecs_field(&qit, WindowComponent, 0);

                    for (int i = 0; i < qit.count; ++i) {
                        if (w[i].window_ptr == closed_window) {
                            ecs_emit(it->world, &(ecs_event_desc_t) {
                                .event = WindowDestroyEvent,
                                    .entity = qit.entities[i],
                                    .ids = &(ecs_type_t) {
                                    .array = (ecs_id_t[]){ ecs_id(WindowComponent) },
                                        .count = 1,
                                },
                            });
                            ecs_delete(it->world, qit.entities[i]);
                        }
                    }
                }

                VD_LOG("Editor", "Destroy window");
                SDL_DestroyWindow(closed_window);

                if (remaining_windows == 0) {
                    ecs_emit(it->world, &(ecs_event_desc_t) {
                        .event = AppQuitEvent,
                            .entity = ecs_id(Application),
                            .ids = &(ecs_type_t) {
                            .array = (ecs_id_t[]){ ecs_id(Application) },
                                .count = 1,
                        },
                    });
                }
            } break;
        }
    }
}

void get_required_extensions(u32 *num_extensions, const char ***extensions)
{
    ensure_sdl_initialized();
    SDL_Window *dummy_window = SDL_CreateWindow("fake", 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    *extensions = SDL_Vulkan_GetInstanceExtensions(num_extensions);
    SDL_DestroyWindow(dummy_window);
}


void SdlImport(ecs_world_t *world)
{
    ensure_sdl_initialized();
    ECS_MODULE(world, Sdl);

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "PollSDLEventsSystem",
            .add = ecs_ids(ecs_dependson(EcsOnLoad))
        }),
        .multi_threaded = false,
        .callback = PollSDLEvents
    });

    ecs_observer(world, {
        .query = {.terms = {{.id = ecs_id(WindowComponent) }}},
        .events = { EcsOnAdd },
        .callback = OnAddWindowObserver,
    });

    ecs_observer(world, {
        .query = {.terms = {
            {.id = EcsName },
            {.id = ecs_id(WindowComponent) },
        }},
        .events = { EcsOnSet },
        .callback = OnSetWindowNameObserver,
    });

}
