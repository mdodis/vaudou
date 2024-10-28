#ifndef VD_IMGUI_MODULE_INTERNAL_H
#define VD_IMGUI_MODULE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *backend_ptr;
    size_t backend_size;
} InitInfo;

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

#ifdef __cplusplus
}
#endif

#endif // !VD_IMGUI_MODULE_INTERNAL_H
