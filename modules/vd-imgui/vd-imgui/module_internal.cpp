#include "module_internal.h"
#include "imgui/imgui.h"

void init_imgui(InitInfo *info)
{

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    
    void *backend_data = IM_ALLOC(info->backend_size);
    memset(backend_data, 0, info->backend_size);

    io.BackendPlatformName = "vaudou";
    io.BackendPlatformUserData = backend_data;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    io.BackendRendererName = "vaudou";

    ImGuiViewport *main_viewport = ImGui::GetMainViewport();

}

void *get_backend_data()
{
    return ImGui::GetIO().BackendPlatformUserData;
}

void *get_fonts_texture_data(int *width, int *height)
{
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *result;
    io.Fonts->GetTexDataAsRGBA32(&result, width, height);
    return result;
}

void begin_frame(BeginFrameInfo *info)
{
    ImGuiIO &io = ImGui::GetIO();
    io.DeltaTime = info->delta_time;
    io.DisplaySize = ImVec2(info->w, info->h);
    io.DisplayFramebufferScale = ImVec2(info->rw, info->rh);

    ImGui::NewFrame();
}

void end_frame()
{
    ImGui::EndFrame();
}

void render()
{
    ImGui::Render();
}
