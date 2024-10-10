#include "module_internal.h"
#include "imgui/imgui.h"

struct BackendData {
    int a;
};

void init_imgui()
{

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    
    BackendData *backend_data = IM_NEW(BackendData)();

    io.BackendPlatformName = "vaudou";
    io.BackendPlatformUserData = (void*)backend_data;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    io.BackendRendererName = "vaudou";

    ImGuiViewport *main_viewport = ImGui::GetMainViewport();

}