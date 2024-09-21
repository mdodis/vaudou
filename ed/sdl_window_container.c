#include "sdl_window_container.h"
#include <assert.h>
#include <stdlib.h>
#include "common.h"
#include "SDL3/SDL_vulkan.h"

ECS_COMPONENT_DECLARE(SDLWindow);

typedef struct {
	ecs_query_t *q_windows;
} SDLWindowContainerState;

VD_WINDOW_CONTAINER_CREATE_WINDOW_PROC(sdl_create_window)
{
	return SDL_CreateWindow(title, 1600, 900, SDL_WINDOW_VULKAN);
}

VD_WINDOW_CONTAINER_DESTROY_WINDOW_PROC(sdl_destroy_window)
{
}

VD_WINDOW_CONTAINER_POLL_PROC(sdl_poll)
{
	SDLWindowContainerState *state = (SDLWindowContainerState *)usrdata;

	SDL_Event evt;
	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			{
				SDL_Window *window_ptr = SDL_GetWindowFromID(evt.window.windowID);
			} break;
		}
	}
}

static void OnAddWindow(ecs_iter_t *it)
{
	for (int i = 0; i < it->count; ++i) {
		const char *name = ecs_get_name(it->world, it->entities[i]);

		SDL_Window *window_ptr = SDL_CreateWindow(name, 640, 480, SDL_WINDOW_VULKAN);
		ecs_set(it->world, it->entities[i], SDLWindow, { .handle = window_ptr });
	}
}

static void OnSetWindowName(ecs_iter_t *it)
{
	SDLWindow *w = ecs_field(it, SDLWindow, 1);

	for (int i = 0; i < it->count; ++i) {
		const char *name = ecs_get_name(it->world, it->entities[i]);

		SDL_SetWindowTitle(w[i].handle, name);
	}
}

static void PollSDLEvents(ecs_iter_t *it)
{
	Application *app = ecs_singleton_get(it->world, Application);

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
						{.id = ecs_id(SDLWindow), },
					}
				});

				ecs_iter_t qit = ecs_query_iter(it->world, q);
				int remaining_windows = qit.count;
				while (ecs_query_next(&qit)) {
					SDLWindow *w = ecs_field(&qit, SDLWindow, 0);

					for (int i = 0; i < qit.count; ++i) {
						if (w[i].handle == closed_window) {
							ecs_delete(it->world, qit.entities[i]);
						}
					}
				}

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
	SDL_Window *dummy_window = SDL_CreateWindow("fake", 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
	*extensions = SDL_Vulkan_GetInstanceExtensions(num_extensions);
	SDL_DestroyWindow(dummy_window);
}


void SdlImport(ecs_world_t *world)
{
	SDLWindowContainerState *state = (SDLWindowContainerState *)malloc(sizeof(SDLWindowContainerState));
	assert(SDL_Init(SDL_INIT_VIDEO) == SDL_TRUE);
	Application *window_container = ecs_singleton_get(world, Application);
	ecs_singleton_set(world, Application, {
		.create_window = sdl_create_window,
		.destroy_window = sdl_destroy_window,
		.poll = sdl_poll,
		.usrdata = state,
	});

	ECS_MODULE(world, Sdl);
	ECS_COMPONENT_DEFINE(world, SDLWindow);

	ecs_system(world, {
		.entity = ecs_entity(world, {
			.name = "PollSDLEvents",
			.add = ecs_ids(ecs_dependson(EcsOnLoad))
		}),
		.multi_threaded = false,
		.callback = PollSDLEvents
	});

	ecs_observer(world, {
		.query = {.terms = {{.id = ecs_id(Window) }}},
		.events = { EcsOnAdd },
		.callback = OnAddWindow,
	});

	ecs_observer(world, {
		.query = {.terms = {
			{.id = EcsName },
			{.id = ecs_id(SDLWindow) },
		}},
		.events = { EcsOnSet },
		.callback = OnSetWindowName,
	});

}
