#ifndef SDL_WINDOW_CONTAINER_H
#define SDL_WINDOW_CONTAINER_H
#include "common.h"
#include "builtin.h"
#include "SDL3/SDL.h"

#include "instance.h"
void get_required_extensions(u32 *num_extensions, const char ***extensions);

VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(sdl_get_physical_device_presentation_support_proc);

extern void SdlImport(ecs_world_t *world);

#endif // !SDL_WINDOW_CONTAINER_H