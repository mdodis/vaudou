#ifndef VD_IMGUI_H
#define VD_IMGUI_H
#include "flecs.h"

typedef struct {
    void (*build_gui)(void *c);
} ImGuiPanel;
extern ECS_COMPONENT_DECLARE(ImGuiPanel);

extern void VdImGuiImport(ecs_world_t *world);
#endif // !VD_IMGUI_H
