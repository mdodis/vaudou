#define VD_INTERNAL_SOURCE_FILE 1
#include "renderer.h"

#include "ng_cfg.h"
#include "instance.h"
#include "builtin.h"
#include "mm.h"
#include "vulkan_helpers.h"

#include "vd_fmt.h"
#include "vd_log.h"
#include "flecs.h"
#include "array.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


struct VD_Renderer {
    ecs_world_t                         *world;
    VD_Instance                         *app_instance;
    VkInstance                          instance;
    PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

    VkPhysicalDevice                    physical_device;
    VkDevice                            device;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } graphics;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } presentation;

#if VD_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT            debug_messenger;
#endif
};

const char *Validation_Layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    VD_LOG_FMT("Vulkan", "%{cstr}", callback_data->pMessage);
    return VK_FALSE;
}

VD_Renderer *vd_renderer_create()
{
    return (VD_Renderer*)calloc(1, sizeof(VD_Renderer));
}

int vd_renderer_init(VD_Renderer *renderer, VD_RendererInitInfo *info)
{
    renderer->app_instance = info->instance;
    renderer->world = info->world;

    VD_VK_CHECK(volkInitialize());

    u32 version = volkGetInstanceVersion();
    VD_LOG_FMT(
            "Renderer", 
            "Vulkan Version: %{u32}.%{u32}.%{u32}", 
            VK_VERSION_MAJOR(version), 
            VK_VERSION_MINOR(version), 
            VK_VERSION_PATCH(version));

    u32 num_enabled_layers = 0;
    const char **enabled_layers = 0;

#if VD_VALIDATION_LAYERS
    VD_LOG("Renderer", "Renderer is built with validation layers enabled.");
#endif

#if VD_VALIDATION_LAYERS
    u32 num_avail_layers = 0;
    vkEnumerateInstanceLayerProperties(&num_avail_layers, 0);

    VkLayerProperties *avail_layers = VD_MM_FRAME_ALLOC_ARRAY(VkLayerProperties, num_avail_layers);
    vkEnumerateInstanceLayerProperties(&num_avail_layers, avail_layers);

    for (int i = 0; i < ARRAY_COUNT(Validation_Layers); ++i) {
        int found = 0;

        for (int j = 0; j < num_avail_layers; ++j) {
            if (strcmp(avail_layers[j].layerName, Validation_Layers[i]) == 0) {
                found = 1;
                break;
            }
        }

        assert(found);
    }

    num_enabled_layers = ARRAY_COUNT(Validation_Layers);
    enabled_layers = Validation_Layers;
#endif


    u32 num_enabled_extensions = 0;
    const char **enabled_extensions = 0;

    enabled_extensions = VD_MM_FRAME_ALLOC_ARRAY(
        const char*,
        info->vulkan.num_enabled_extensions + 1);

    for (int i = 0; i < info->vulkan.num_enabled_extensions; ++i) {
        enabled_extensions[num_enabled_extensions++] = info->vulkan.enabled_extensions[i];
    }

#if VD_VALIDATION_LAYERS
    enabled_extensions[num_enabled_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

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
        },
        0,
        &renderer->instance));

    volkLoadInstance(renderer->instance);

#if VD_VALIDATION_LAYERS
    renderer->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(
            renderer->instance,
            "vkCreateDebugUtilsMessengerEXT");

    renderer->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(
            renderer->instance,
            "vkDestroyDebugUtilsMessengerEXT");

    assert(renderer->vkCreateDebugUtilsMessengerEXT);
    assert(renderer->vkDestroyDebugUtilsMessengerEXT);
    
    renderer->vkCreateDebugUtilsMessengerEXT(
        renderer->instance,
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
        &renderer->debug_messenger);
#endif

    // ----PICK PHYSICAL DEVICE---------------------------------------------------------------------
    dynarray VkPhysicalDevice *physical_devices = 0;
    array_init(physical_devices, VD_MM_FRAME_ALLOCATOR());
    {
        
        u32 num_physical_devices = 0;
        vkEnumeratePhysicalDevices(renderer->instance, &num_physical_devices, 0);

        array_addn(physical_devices, num_physical_devices);

        vkEnumeratePhysicalDevices(renderer->instance, &num_physical_devices, physical_devices);
    }

    int best_device = -1;
    u32 best_device_graphics_queue_family = 0;
    u32 best_device_present_queue_family = 0;
    int best_device_is_gpu = 0;

    for (int i = 0; i < array_len(physical_devices); ++i) {
        int q_device                                = i;
        int q_device_graphics_queue_family_present  = 0;
        u32 q_device_graphics_queue_family          = 0;
        int q_device_present_queue_family_present   = 0;
        u32 q_device_present_queue_family           = 0;
        int q_device_is_gpu                         = 0;

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
        array_init(queue_families, VD_MM_FRAME_ALLOCATOR());
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
            "Renderer",
            "Device[%{i32}]: %{cstr}, API Version: %{u32}.%{u32}.%{u32}",
            i,
            props.properties.deviceName,
            VK_VERSION_MAJOR(props.properties.apiVersion),
            VK_VERSION_MINOR(props.properties.apiVersion),
            VK_VERSION_PATCH(props.properties.apiVersion));

        VD_LOG_FMT(
            "Renderer",
            "\tQueue Families: %{u32}",
            array_len(queue_families));

        dynarray u32 *graphics_queues = 0;
        array_init(graphics_queues, VD_MM_FRAME_ALLOCATOR());

        dynarray u32 *present_queues = 0;
        array_init(present_queues , VD_MM_FRAME_ALLOCATOR());

        for (int j = 0; j < array_len(queue_families); ++j) {
            VkQueueFamilyProperties *queue_family_properties = &queue_families[j];
            int queue_surface_support = info->vulkan.get_physical_device_presentation_support(
                &renderer->instance,
                &physical_devices[i],
                j,
                info->vulkan.usrdata);

            if (queue_family_properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                array_add(graphics_queues, j);
                VD_LOG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_GRAPHICS_BIT",
                    j);
            }

            if (queue_family_properties->queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                VD_LOG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_COMPUTE_BIT",
                    j);
            }

            if (queue_family_properties->queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                VD_LOG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] VK_QUEUE_TRANSFER_BIT",
                    j);
            }

            if (queue_surface_support) {

                VD_LOG_FMT(
                    "Renderer",
                    "\t\tQueue[%{i32}] Supports Presentation",
                    j);

                array_add(present_queues, j);
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

        if (q_device_graphics_queue_family == q_device_present_queue_family && (array_len(present_queues) > 1))
        {
            q_device_present_queue_family = present_queues[1];
        }

        VD_LOG_FMT(
            "Renderer",
            "\tMulti draw indirect: %{u32}",
            features.features.multiDrawIndirect);

        VD_LOG_FMT(
            "Renderer",
            "\tBuffer Device Address (1.2): %{u32}",
            features12.bufferDeviceAddress);

        VD_LOG_FMT(
            "Renderer",
            "\tDescriptor Indexing (1.2): %{u32}",
            features12.descriptorIndexing);

        VD_LOG_FMT(
            "Renderer",
            "\tSynchronization 2 (1.3): %{u32}",
            features13.synchronization2);

        VD_LOG_FMT(
            "Renderer",
            "\tDynamic Rendering (1.3): %{u32}",
            features13.dynamicRendering);

        int physical_device_satisfies_requirements = 
            q_device_graphics_queue_family_present &&
            q_device_present_queue_family_present &&
            features12.bufferDeviceAddress &&
            features12.descriptorIndexing &&
            features13.synchronization2 &&
            features13.dynamicRendering;

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

    VD_LOG_FMT("Renderer", "Best device: %{i32}", best_device);
    VD_LOG_FMT("Renderer", "\tGraphics Queue: %{u32}", best_device_graphics_queue_family);
    VD_LOG_FMT("Renderer", "\tPresent Queue: %{u32}", best_device_present_queue_family);

    // ----CREATE LOGICAL DEVICE--------------------------------------------------------------------
    VD_VK_CHECK(vkCreateDevice(
        physical_devices[best_device],
        & (VkDeviceCreateInfo) 
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount       = 2,
            .pQueueCreateInfos = (VkDeviceQueueCreateInfo[2]) 
            {
                {
                    .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex   = best_device_graphics_queue_family,
                    .queueCount         = 1,
                    .pQueuePriorities   = (float[]) { 1.0f },
                },
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex   = best_device_present_queue_family,
                    .queueCount         = 1,
                    .pQueuePriorities   = (float[]) { 1.0f },
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
            }
        },
        0,
        &renderer->device));
    return 0;
}

void RendererOnWindowComponentSet(ecs_iter_t *it)
{
    const Application *app = ecs_singleton_get(it->world, Application);
    VD_Renderer *renderer = vd_instance_get_renderer(app->instance);

    WindowComponent *w = ecs_field(it, WindowComponent, 0);
    
    for (int i = 0; i < it->count; ++i) {
        if (w[i].create_surface == 0) {
            continue;
        }

        VkSurfaceKHR surface = w[i].create_surface(&w[i], renderer->instance);
    }
}

int vd_renderer_deinit(VD_Renderer *renderer)
{
    vkDestroyDevice(renderer->device, 0);
#if VD_VALIDATION_LAYERS
    renderer->vkDestroyDebugUtilsMessengerEXT(renderer->instance, renderer->debug_messenger, 0);
#endif
    vkDestroyInstance(renderer->instance, 0);
    return 0;
}