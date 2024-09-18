#include "builtin.h"
#include "SDL3/SDL.h"

typedef struct {
	SDL_Window *handle;
} SDLWindow;

extern ECS_COMPONENT_DECLARE(SDLWindow);
extern void SdlImport(ecs_world_t *world);