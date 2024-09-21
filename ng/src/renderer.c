#define VD_INTERNAL_SOURCE_FILE 1

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "renderer.h"

#include "volk.h"

#include "vulkan/vk_enum_string_helper.h"

const char *Validation_Layers[] = {
	"VK_LAYER_KHRONOS_validation",
};

typedef struct VD_Instance VD_Instance;

struct VD_Renderer {
	VD_Instance *app_instance;
	VkInstance instance;
};

VD_Renderer *vd_renderer_create()
{
	return (VD_Renderer*)calloc(1, sizeof(VD_Renderer));
}

int vd_renderer_init(VD_Renderer *renderer, VD_RendererInitInfo *info)
{
	renderer->app_instance = info->instance;

	VD_VK_CHECK(volkInitialize());

	u32 version = volkGetInstanceVersion();
	printf("Vulkan version %d.%d.%d initialized.\n",
		VK_VERSION_MAJOR(version),
		VK_VERSION_MINOR(version),
		VK_VERSION_PATCH(version));

	u32 num_enabled_layers;
	const char **enabled_layers;

	VD_VK_CHECK(vkCreateInstance(
		&(VkInstanceCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &(VkApplicationInfo) 
			{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Vaudou",
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_API_VERSION_1_2,
			},
			.ppEnabledExtensionNames = info->vulkan.enabled_extensions,
			.enabledExtensionCount = info->vulkan.num_enabled_extensions,
			.enabledLayerCount = 0,

		},
		0,
		&renderer->instance));

	volkLoadInstance(renderer->instance);

	return 0;
}

int vd_renderer_deinit(VD_Renderer *renderer)
{
	vkDestroyInstance(renderer->instance, 0);
	return 0;
}