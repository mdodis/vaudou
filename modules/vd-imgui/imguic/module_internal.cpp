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
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

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

DrawList get_draw_list()
{
    ImDrawData *draw_data = ImGui::GetDrawData();
    if (draw_data == 0 || draw_data->CmdListsCount == 0) {
        return 0;
    }

    return draw_data;
}

size_t draw_list_indx_count(DrawList list)
{
    return ((ImDrawData*)list)->TotalIdxCount;
}

size_t draw_list_vert_count(DrawList list)
{
    return ((ImDrawData*)list)->TotalVtxCount;
}

size_t draw_list_cmdlist_count(DrawList list)
{
    return ((ImDrawData*)list)->CmdListsCount;
}

CmdList draw_list_get_cmdlist(DrawList list, size_t i)
{
    return (CmdList)((ImDrawData*)list)->CmdLists[i];
}

unsigned int cmd_list_indx_count(CmdList list)
{
    return ((ImDrawList*)list)->IdxBuffer.size();
}

unsigned short *cmd_list_indx_ptr(CmdList list)
{
    return ((ImDrawList*)list)->IdxBuffer.begin();
}

unsigned int cmd_list_vert_count(CmdList list)
{
    return ((ImDrawList*)list)->VtxBuffer.size();
}

void cmd_list_vert_pos(CmdList list, unsigned int i, float *pos)
{
    memcpy(pos, &((ImDrawList*)list)->VtxBuffer[i].pos, sizeof(float) * 2);
}

void cmd_list_vert_tex(CmdList list, unsigned int i, float *tex)
{
    memcpy(tex, &((ImDrawList*)list)->VtxBuffer[i].uv, sizeof(float) * 2);
}

void cmd_list_vert_col(CmdList list, unsigned int i, float *col)
{
    memcpy(col, &((ImDrawList*)list)->VtxBuffer[i].col, sizeof(float) * 4);
}

unsigned int cmd_list_buf_count(CmdList list)
{
    return ((ImDrawList*)list)->CmdBuffer.size();
}

CmdBuffer cmd_list_buf(CmdList list, unsigned int i)
{
    return &((ImDrawList*)list)->CmdBuffer[i];
}

unsigned int cmd_buffer_vtx_offset(CmdBuffer buf)
{
    return ((ImDrawCmd*)buf)->VtxOffset;
}

unsigned int cmd_buffer_idx_offset(CmdBuffer buf)
{
    return ((ImDrawCmd*)buf)->IdxOffset;
}

unsigned int cmd_buffer_idx_count(CmdBuffer buf)
{
    return ((ImDrawCmd*)buf)->ElemCount;
}

void cmd_buffer_clip(CmdBuffer buf, float *clip4)
{
    memcpy(clip4, &((ImDrawCmd*)buf)->ClipRect, sizeof(float) * 4);
}

void imgui_text(const char *text)
{
    ImGui::Text("%s", text);
}
