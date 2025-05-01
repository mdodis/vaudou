#define VD_INTERNAL_SOURCE_FILE 1
#include "array.h"
#include "rhi_vulkan.h"
#include "vd_log.h"
#include "volk.h"

#include "tracy/TracyC.h"

static VD_RHI_INIT_PROC(rhi_vulkan_init);

static const char *Validation_Layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

static const char *Debug_Extensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

static const char *Window_Extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if VD_PLATFORM_WINDOWS
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif VD_PLATFORM_MACOS
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#else
#error "Unsupported platform!"
#endif
};

typedef struct {
    VkInstance                          instance;
    VkPhysicalDevice                    physical_device;
    VkDevice                            device;
    VD_Allocator                        *frame_allocator;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } graphics;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } presentation;

    struct {
        // Debug messenger
        PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
        VkDebugUtilsMessengerEXT            messenger;

        // Name object
        PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
    } debug;
} VulkanContext;

static VulkanContext Vulkan_Context;
#define CTX() (&Vulkan_Context)

void rhi_vulkan_populate(VD(RHI) *rhi)
{
    rhi->c = &Vulkan_Context;
    rhi->initialize = rhi_vulkan_init;
}

static inline const char* vkresult_to_string(VkResult input_value);
static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

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
    // 3. Window_Extensions (only if VD(RHInitInfo).extensions.headless is "0").

    // 1.
    num_enabled_extensions = num_required_extensions;

    // 2.
    if (info->extensions.debug) {
        num_enabled_extensions += ARRAY_COUNT(Debug_Extensions);
    }

    // 3.
    if (!info->extensions.headless) {
        num_enabled_extensions += ARRAY_COUNT(Window_Extensions);
    }

    enabled_extensions = VD_ALLOC_ARRAY(
        CTX()->frame_allocator,
        const char *,
        num_enabled_extensions);

    u32 num_current_extensions = 0;

    // 1. Copy over required instance extensions
    for (int i = 0; i < num_required_extensions; ++i) {
        enabled_extensions[num_current_extensions] = 
            info->extensions.api_specific.vulkan.instance_extensions[i];

        num_current_extensions++;
    }

    // 2. Copy over debug extensions (if in debug mode)
    if (info->extensions.debug) {
        for (int i = 0; i < ARRAY_COUNT(Debug_Extensions); ++i) {
            enabled_extensions[num_current_extensions] = Debug_Extensions[i];
            num_current_extensions++;
        }
    }

    // 3. Copy over window extensions (if not in headless mode)
    if (!info->extensions.headless) {
        for (int i = 0; i < ARRAY_COUNT(Window_Extensions); ++i) {
            enabled_extensions[num_current_extensions] = Window_Extensions[i];
            num_current_extensions++;
        }
    }

    {
        u32 num_available_extensions;
        VD_VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &num_available_extensions, 0));

        VkExtensionProperties *props = (VkExtensionProperties*)vd_malloc(
            vd_memory_get_system_allocator(),
            sizeof(VkExtensionProperties) * num_available_extensions);

        VD_VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &num_available_extensions, props));

        VD_LOG_FMT("RHI", "Listing %{u32} extensions", num_available_extensions);
        for (int i = 0; i < num_available_extensions; ++i) {
            VD_LOG_FMT("RHI", "Instance Ext: %{cstr}", props[i].extensionName);
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
        for (int i = 0; i < num_avail_layers; ++i) {
            VD_LOG_FMT("RHI", "Available Layer: %{cstr}", avail_layers[i].layerName);
        }

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

    if (info->extensions.debug) {
        // Create debug messenger
        CTX()->debug.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(
                CTX()->instance,
                "vkCreateDebugUtilsMessengerEXT");

        CTX()->debug.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(
                CTX()->instance,
                "vkDestroyDebugUtilsMessengerEXT");

        CTX()->debug.vkCreateDebugUtilsMessengerEXT(
            CTX()->instance,
            & (VkDebugUtilsMessengerCreateInfoEXT)
            {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = vk_debug_callback,
                .pUserData = 0,
            },
            0,
            &CTX()->debug.messenger);

        // Load name object utility
        CTX()->debug.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)
            vkGetInstanceProcAddr(
                CTX()->instance,
                "vkSetDebugUtilsObjectNameEXT");
    }

// ----PICK PHYSICAL DEVICE-------------------------------------------------------------------------
    
    dynarray VkPhysicalDevice *physical_devices = 0;
    array_init(physical_devices, info->frame_allocator);
    {
        
        u32 num_physical_devices = 0;
        vkEnumeratePhysicalDevices(CTX()->instance, &num_physical_devices, 0);

        array_addn(physical_devices, num_physical_devices);

        vkEnumeratePhysicalDevices(CTX()->instance, &num_physical_devices, physical_devices);
    }

    int best_device = -1;
    u32 best_device_graphics_queue_family = 0;
    u32 best_device_present_queue_family = 0;
    int best_device_is_gpu = 0;
    int min_major_version = 1;
    int min_minor_version = 3;

    for (int i = 0; i < array_len(physical_devices); ++i) {
        int q_device                                = i;
        int q_device_graphics_queue_family_present  = 0;
        u32 q_device_graphics_queue_family          = 0;
        int q_device_present_queue_family_present   = 0;
        u32 q_device_present_queue_family           = 0;
        int q_device_is_gpu                         = 0;
        int q_device_supports_swapchain             = 0;

        dynarray VkExtensionProperties *device_extensions = 0;
        array_init(device_extensions, info->frame_allocator);
        {
            u32 num_device_extensions = 0;
            vkEnumerateDeviceExtensionProperties(physical_devices[i], 0, &num_device_extensions, 0);

            array_addn(device_extensions, num_device_extensions);
            vkEnumerateDeviceExtensionProperties(
                physical_devices[i],
                0,
                &num_device_extensions,
                device_extensions);

        }

        VkPhysicalDeviceProperties2 props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        };
        vkGetPhysicalDeviceProperties2(physical_devices[i], &props);

        q_device_is_gpu = props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        VkPhysicalDeviceVulkan12Features features12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };

        VkPhysicalDeviceVulkan13Features features13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &features12,
        };

        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &features13,
        };
        vkGetPhysicalDeviceFeatures2(physical_devices[i], &features);

        dynarray VkQueueFamilyProperties *queue_families = 0;
        array_init(queue_families, info->frame_allocator);
        {
            u32 num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &num_queue_families, 0);

            array_addn(queue_families, num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(
                physical_devices[i],
                &num_queue_families, 
                queue_families);
        }

        VD_LOG_FMT(
            "RHI",
            "Device[%{i32}]: %{cstr}, API Version: %{u32}.%{u32}.%{u32}",
            i,
            props.properties.deviceName,
            VK_VERSION_MAJOR(props.properties.apiVersion),
            VK_VERSION_MINOR(props.properties.apiVersion),
            VK_VERSION_PATCH(props.properties.apiVersion));

        VD_DBG_FMT(
            "RHI",
            "\tExtensions: %{u32}",
            array_len(device_extensions));

        for (int j = 0; j < array_len(device_extensions); ++j) {
            VD_DBG_FMT(
                "RHI",
                "\t\t%{cstr}(%{u32}.%{u32}.%{u32})",
                device_extensions[j].extensionName,
                VK_VERSION_MAJOR(device_extensions[j].specVersion),
                VK_VERSION_MINOR(device_extensions[j].specVersion),
                VK_VERSION_PATCH(device_extensions[j].specVersion));

            if (strcmp(device_extensions[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                q_device_supports_swapchain = 1;
            }
        }

        VD_DBG_FMT(
            "RHI",
            "\tQueue Families: %{u32}",
            array_len(queue_families));

        dynarray u32 *graphics_queues = 0;
        array_init(graphics_queues, info->frame_allocator);

        dynarray u32 *present_queues = 0;
        array_init(present_queues , info->frame_allocator);

        for (int j = 0; j < array_len(queue_families); ++j) {
            VkQueueFamilyProperties *queue_family_properties = &queue_families[j];
            if (queue_family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                array_add(graphics_queues, j);
                VD_DBG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_GRAPHICS_BIT",
                    j);
            }

            if (queue_family_properties->queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                VD_DBG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_COMPUTE_BIT",
                    j);
            }

            if (queue_family_properties->queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                VD_DBG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_TRANSFER_BIT",
                    j);
            }

            if (!info->extensions.headless) {
                int queue_surface_support;
                RHResult result = info->is_physical_device_suitable(
                    rhi,
                    physical_devices[i],
                    &queue_surface_support,
                    info->c);

                if (result != RH_RESULT_SUCCESS) {
                    return result;
                }


                if (queue_surface_support) {
                    array_add(present_queues, j);
                    VD_DBG_FMT(
                        "Renderer",
                        "\t\tQueue[%{i32}] Supports Presentation",
                        j);
                }
            }
        }

        q_device_graphics_queue_family_present = array_len(graphics_queues) > 0;
        q_device_present_queue_family_present = array_len(present_queues) > 0;

        q_device_graphics_queue_family = q_device_graphics_queue_family_present
            ? graphics_queues[0]
            : 0;

        q_device_present_queue_family = q_device_present_queue_family_present
            ? present_queues[0]
            : 0;

        if (q_device_graphics_queue_family == q_device_present_queue_family &&
            (array_len(present_queues) > 1))
        {
            q_device_present_queue_family = present_queues[1];
        }

        VD_DBG_FMT(
            "Renderer",
            "\tMulti draw indirect: %{u32}",
            features.features.multiDrawIndirect);

        VD_DBG_FMT(
            "Renderer",
            "\tBuffer Device Address (1.2): %{u32}",
            features12.bufferDeviceAddress);

        VD_DBG_FMT(
            "Renderer",
            "\tDescriptor Indexing (1.2): %{u32}",
            features12.descriptorIndexing);

        VD_DBG_FMT(
            "Renderer",
            "\tSynchronization 2 (1.3): %{u32}",
            features13.synchronization2);

        VD_DBG_FMT(
            "Renderer",
            "\tDynamic Rendering (1.3): %{u32}",
            features13.dynamicRendering);

        int physical_device_satisfies_requirements = 
            q_device_supports_swapchain &&
            q_device_graphics_queue_family_present &&
            (info->extensions.headless || q_device_present_queue_family_present) &&
            features12.bufferDeviceAddress &&
            features12.descriptorIndexing &&
            features13.synchronization2 &&
            features13.dynamicRendering &&
            VK_VERSION_MAJOR(props.properties.apiVersion) >= min_major_version &&
            VK_VERSION_MINOR(props.properties.apiVersion) >= min_minor_version;

        if (best_device == -1)
        {
            best_device                         = i;
            best_device_graphics_queue_family   = q_device_graphics_queue_family;
            best_device_present_queue_family    = q_device_present_queue_family;
            best_device_is_gpu                  = q_device_is_gpu;
        } else if (q_device_is_gpu && physical_device_satisfies_requirements) {
            best_device                         = i;
            best_device_graphics_queue_family   = q_device_graphics_queue_family;
            best_device_present_queue_family    = q_device_present_queue_family;
            best_device_is_gpu                  = q_device_is_gpu;
        }
    }

    VD_DBG_FMT("RHI", "Best device: %{i32}", best_device);
    VD_DBG_FMT("RHI", "\tGraphics Queue: %{u32}", best_device_graphics_queue_family);
    VD_DBG_FMT("RHI", "\tPresent Queue: %{u32}", best_device_present_queue_family);

    CTX()->physical_device = physical_devices[best_device];

// ----CREATE LOGICAL DEVICE------------------------------------------------------------------------

    static const char *device_window_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if VD_PLATFORM_MACOS
        "VK_KHR_portability_subset"
#endif
    };

    const char **create_logical_device_extensions = 0;
    u32 num_create_logical_device_extensions = 0;

    if (!info->extensions.headless) {
        create_logical_device_extensions = device_window_extensions;
        num_create_logical_device_extensions = ARRAY_COUNT(device_window_extensions);
    }
    
    VD_VK_CHECK(vkCreateDevice(
        CTX()->physical_device,
        & (VkDeviceCreateInfo) 
        {
            .sType                          = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount           = info->extensions.headless ? 1 : 2,
            .pQueueCreateInfos = (VkDeviceQueueCreateInfo[2]) 
            {
                {
                    .sType                  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex       = best_device_graphics_queue_family,
                    .queueCount             = 1,
                    .pQueuePriorities       = (float[]) { 1.0f },
                },
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex       = best_device_present_queue_family,
                    .queueCount             = 1,
                    .pQueuePriorities       = (float[]) { 1.0f },
                }
            },
            .pNext = & (VkPhysicalDeviceFeatures2) 
            {
                .sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .features = (VkPhysicalDeviceFeatures) 
                {
                    .multiDrawIndirect      = VK_TRUE,
                },
                .pNext = & (VkPhysicalDeviceVulkan12Features) 
                {
                    .sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                    .bufferDeviceAddress    = VK_TRUE,
                    .descriptorIndexing     = VK_TRUE,
                    .pNext = & (VkPhysicalDeviceVulkan13Features) 
                    {
                        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                        .synchronization2   = VK_TRUE,
                        .dynamicRendering   = VK_TRUE,
                    },
                },
            },
            .enabledExtensionCount          = num_create_logical_device_extensions,
            .ppEnabledExtensionNames        = create_logical_device_extensions,
        },
        0,
        &CTX()->device));
    volkLoadDevice(CTX()->device);

    CTX()->graphics.queue_family_index = best_device_graphics_queue_family;
    CTX()->presentation.queue_family_index = best_device_present_queue_family;

    vkGetDeviceQueue(
        CTX()->device,
        best_device_graphics_queue_family,
        0,
        &CTX()->graphics.queue);

    vkGetDeviceQueue(
        CTX()->device,
        best_device_present_queue_family,
        0,
        &CTX()->presentation.queue);

    return RH_RESULT_SUCCESS;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            VD_DBG_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            VD_DBG_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            VD_WRN_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            VD_ERR_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
            break;
        default:
            VD_LOG_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
            break;
    }
    return VK_FALSE;
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
