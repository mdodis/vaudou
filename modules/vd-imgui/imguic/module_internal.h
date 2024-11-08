#ifndef VD_IMGUI_MODULE_INTERNAL_H
#define VD_IMGUI_MODULE_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct {
    void *backend_ptr;
    size_t backend_size;
} InitInfo;

typedef void *DrawList;
typedef void *CmdList;

void init_imgui(InitInfo *info);

void *get_backend_data();
void *get_fonts_texture_data(int *width, int *height);

typedef struct {
    float delta_time;
    int w, h;
    int rw, rh;
} BeginFrameInfo;

void begin_frame(BeginFrameInfo *info);
void end_frame();

void render();

DrawList get_draw_list();

size_t draw_list_indx_count(DrawList list);
size_t draw_list_vert_count(DrawList list);
size_t draw_list_cmdlist_count(DrawList list);
CmdList draw_list_get_cmdlist(DrawList list, size_t i);

unsigned int cmd_list_indx_count(CmdList list);
unsigned short *cmd_list_indx_ptr(CmdList list);

unsigned int cmd_list_vert_count(CmdList list);
void cmd_list_vert_pos(CmdList list, unsigned int i, float *pos);
void cmd_list_vert_tex(CmdList list, unsigned int i, float *tex);
void cmd_list_vert_col(CmdList list, unsigned int i, float *col);

void imgui_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif // !VD_IMGUI_MODULE_INTERNAL_H
