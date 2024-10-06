#ifndef VD_W_SDL_H
#define VD_W_SDL_H
#include "vd_common.h"
#include "builtin.h"

#include "instance.h"

void vd_w_sdl_get_required_extensions(u32 *num_extensions, const char ***extensions);

VD_GET_PHYSICAL_DEVICE_PRESENTATION_SUPPORT_PROC(
    vd_w_sdl_get_physical_device_presentation_support_proc);

void vd_w_sdl_deinit();

extern void SdlImport(ecs_world_t *world);

#endif // !VD_W_SDL_H