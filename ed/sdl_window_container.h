#ifndef SDL_WINDOW_CONTAINER_H
#define SDL_WINDOW_CONTAINER_H
#include "common.h"
#include "builtin.h"
#include "SDL3/SDL.h"

typedef struct {
	SDL_Window *handle;
} SDLWindow;

void get_required_extensions(u32 *num_extensions, const char ***extensions);

extern ECS_COMPONENT_DECLARE(SDLWindow);
extern void SdlImport(ecs_world_t *world);

#endif // !SDL_WINDOW_CONTAINER_H