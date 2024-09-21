#ifndef VD_RENDERER_H
#define VD_RENDERER_H

typedef struct VD_Instance VD_Instance;
typedef struct VD_Renderer VD_Renderer;

#define VD_VK_CHECK(expr) \
	do { \
		VkResult __result = (expr); \
		if (__result != VK_SUCCESS) { printf("Call " #expr " failed with code: %s\n", string_VkResult(__result)); abort(); } \
	} while(0)

typedef struct {
	VD_Instance		*instance;
	struct {
		u32			num_enabled_extensions;
		const char	**enabled_extensions;
	} vulkan;
} VD_RendererInitInfo;

VD_Renderer *vd_renderer_create();
int vd_renderer_init(VD_Renderer *renderer, VD_RendererInitInfo *info);
int vd_renderer_deinit(VD_Renderer *renderer);


#ifdef VD_ENABLE_VALIDATION_LAYERS
#define VD_VALIDATION_LAYERS 1
#else
#define VD_VALIDATION_LAYERS 0
#endif

#endif // !VD_RENDERER_H