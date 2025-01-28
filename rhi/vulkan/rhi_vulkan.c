#define VD_INTERNAL_SOURCE_FILE 1
#include "rhi_vulkan.h"
#include "vd_log.h"
#include "volk.h"

static VD_RHI_INIT_PROC(rhi_vulkan_init);

static const char *Validation_Layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

static const char *Debug_Extensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

typedef struct {
    VkInstance      instance;
    VD_Allocator    *frame_allocator;
} VulkanContext;

static VulkanContext Vulkan_Context;
#define CTX() (&Vulkan_Context)

void rhi_vulkan_populate(VD(RHI) *rhi)
{
    rhi->c = &Vulkan_Context;
    rhi->initialize = rhi_vulkan_init;
}

static inline const char* vkresult_to_string(VkResult input_value);

#define VD_VK_CHECK(expr) \
	do { \
		VkResult __result = (expr); \
		if (__result != VK_SUCCESS) { VD_LOG_FMT("Vulkan", "Call " #expr " failed with code: %{cstr}\n", vkresult_to_string(__result)); abort(); } \
	} while(0)

static VD_RHI_INIT_PROC(rhi_vulkan_init)
{
    // Initialize loader first.
    CTX()->frame_allocator = info->frame_allocator;
    volkInitialize();

    u32 version = volkGetInstanceVersion();
    VD_LOG_FMT(
        "Renderer", 
        "Vulkan Version: %{u32}.%{u32}.%{u32}", 
        VK_VERSION_MAJOR(version), 
        VK_VERSION_MINOR(version), 
        VK_VERSION_PATCH(version));

    // Validation layers; only enabled if VD(RHInitInfo).extensions.debug is "1".
    u32 num_enabled_layers = 0;
    const char **enabled_layers = 0;

    // General extensions; guaranteed enabled for any extensions specified in
    // VD(RHInitInfo).extensions.api_specific.vulkan.
    u32 num_enabled_extensions = 0;
    const char **enabled_extensions = 0;

    u32 num_required_extensions = info->extensions.api_specific.vulkan.num_instance_extensions;

    // Populate enabled_extensions based on
    // 1. Extensions present in VD(RHInitInfo).extensions.api_specific.vulkan.
    // 2. Debug_Extensions (only if VD(RHInitInfo).extensions.debug is "1").

    // 1.
    num_enabled_extensions = num_required_extensions;

    // 2.
    if (info->extensions.debug) {
        num_enabled_extensions += ARRAY_COUNT(Debug_Extensions);
    }

    enabled_extensions = VD_ALLOC_ARRAY(
        CTX()->frame_allocator,
        const char *,
        num_enabled_extensions);

    // Copy over required instance extensions
    for (int i = 0; i < num_required_extensions; ++i) {
        enabled_extensions[i] = info->extensions.api_specific.vulkan.instance_extensions[i];
    }

    // Copy over debug extensions (if in debug mode)
    if (info->extensions.debug) {
        for (int i = 0; i < ARRAY_COUNT(Debug_Extensions); ++i) {
            enabled_extensions[num_required_extensions +i] = Debug_Extensions[i];
        }
    }

    if (info->extensions.debug) {
        u32 num_avail_layers = 0;
        vkEnumerateInstanceLayerProperties(&num_avail_layers, 0);

        VkLayerProperties *avail_layers = VD_ALLOC_ARRAY(
                CTX()->frame_allocator,
                VkLayerProperties,
                num_avail_layers);
        vkEnumerateInstanceLayerProperties(&num_avail_layers, avail_layers);

        int can_enable_validation_layers = 1;
        for (int i = 0; i < ARRAY_COUNT(Validation_Layers); ++i) {
            int found = 0;

            for (int j = 0; j < num_avail_layers; ++j) {
                if (strcmp(avail_layers[j].layerName, Validation_Layers[i]) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                VD_LOG("Renderer", "Could not find validation layers");
                can_enable_validation_layers = 0;
                break;
            }
        }

        if (can_enable_validation_layers) {
            num_enabled_layers = ARRAY_COUNT(Validation_Layers);
            enabled_layers = Validation_Layers;
        }
    }

    VkInstanceCreateFlags instance_create_flags = 0;

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
                .apiVersion = VK_API_VERSION_1_3,
            },
            .ppEnabledExtensionNames = enabled_extensions,
            .enabledExtensionCount = num_enabled_extensions,
            .ppEnabledLayerNames = enabled_layers,
            .enabledLayerCount = num_enabled_layers,
            .flags = instance_create_flags,
        },
        0,
        &CTX()->instance));

    volkLoadInstance(CTX()->instance);

    return RH_RESULT_SUCCESS;
}

static inline const char* vkresult_to_string(VkResult input_value) {
    switch (input_value) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
#endif //VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
            return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
        default:
            return "Unhandled VkResult";
    }
}
