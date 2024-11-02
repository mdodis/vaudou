// renderer.c
// 
// TODO
// - Move renderer deletion queue to VD_MM_GLOBAL
// - Move window deletion queue to VD_MM_ENTITY
#define VD_INTERNAL_SOURCE_FILE 1
#include "r/sshader.h"
#include "r/geo_system.h"
#include "r/texture_system.h"
#include "r/smat.h"
#include "vd_common.h"
#include "renderer.h"
#include "default_shaders.h"

#define VD_VK_OPTION_INCLUDE_VULKAN_CUSTOM
#define VD_VK_IMPLEMENTATION
#define VD_VK_CUSTOM_CHECK(x) VD_VK_CHECK(x)
#include "vd_vk.h"
#include "vk_mem_alloc.h"

#include "fmt.h"
#include "vd_log.h"
#include "flecs.h"
#include "cglm/project.h"
#include "cglm/affine.h"
#include "array.h"

#include "ng_cfg.h"
#include "instance.h"
#include "builtin.h"
#include "mm.h"
#include "vulkan_helpers.h"
#include "cvar.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "shdc.h"

#include "tracy/TracyC.h"

static void vd_shdc_log_error(const char *what, const char *msg, const char *extmsg);

struct VD_Renderer {
    ecs_world_t                         *world;
    VD_Instance                         *app_instance;
    VkInstance                          instance;

// ----SYSTEMS--------------------------------------------------------------------------------------
    VD_R_TextureSystem                  textures;
    VD_R_GeoSystem                      geos;
    SShader                             sshader;
    SMat                                smat;

// ----RENDERING DEVICES----------------------------------------------------------------------------
    VkPhysicalDevice                    physical_device;
    VkDevice                            device;

    VD_DeletionQueue                    deletion_queue;

    VmaAllocator                        allocator;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } graphics;

    struct {
        u32                             queue_family_index;
        VkQueue                         queue;
    } presentation;

    VkFormat                            color_image_format;
    VkFormat                            depth_image_format;

    struct {
        VkFence                 fence;
        VkCommandBuffer         command_buffer;
        VkCommandPool           command_pool;
    } imm;

    dynarray RenderObject *render_object_list;

// ----PRIMITIVES-----------------------------------------------------------------------------------
    struct {
        HandleOf(VD_R_GPUMesh)  quad;
        HandleOf(VD_R_GPUMesh)  sphere;
        HandleOf(VD_R_GPUMesh)  cube;
    } meshes;

    struct {
        
        HandleOf(VD_R_AllocatedImage) black;
        HandleOf(VD_R_AllocatedImage) white;
        HandleOf(VD_R_AllocatedImage) checker_magenta;
    } images;

    struct {
        HandleOf(GPUMaterialBlueprint)   pbropaque;
    } materials;

#if VD_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT            debug_messenger;
    PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif

#if VD_VULKAN_OBJECT_NAMES
    PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
#endif
};

const VkSurfaceFormatKHR Preferred_Surface_Formats[] = {
    { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
};

const VkPresentModeKHR Preferred_Present_Modes[] = {
    VK_PRESENT_MODE_MAILBOX_KHR,
    VK_PRESENT_MODE_FIFO_KHR,
};

const char *Validation_Layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

_inline void name_object(VD_Renderer *renderer, VkDebugUtilsObjectNameInfoEXT *info)
{
#if VD_VULKAN_OBJECT_NAMES
    renderer->vkSetDebugUtilsObjectNameEXT(renderer->device, info);
#endif
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

VD_Renderer *vd_renderer_create()
{
    return (VD_Renderer*)calloc(1, sizeof(VD_Renderer));
}

static const char *Num_Extra_Instance_Extensions[] = {
    // Debug Utils
#if VD_VALIDATION_LAYERS
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif

    // MoltenVK
#if VD_PLATFORM_MACOS
    "VK_KHR_portability_enumeration",
#endif
};

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
#endif


    u32 num_enabled_extensions = 0;
    const char **enabled_extensions = 0;

    enabled_extensions = VD_MM_FRAME_ALLOC_ARRAY(
        const char*,
        info->vulkan.num_enabled_extensions + ARRAY_COUNT(Num_Extra_Instance_Extensions));

    for (int i = 0; i < info->vulkan.num_enabled_extensions; ++i) {
        enabled_extensions[num_enabled_extensions++] = info->vulkan.enabled_extensions[i];
    }

    for (int i = 0; i < ARRAY_COUNT(Num_Extra_Instance_Extensions); ++i) {
        VD_DBG_FMT(
            "Renderer",
            "Enabling instance extension: %{cstr}",
            Num_Extra_Instance_Extensions[i]);
        enabled_extensions[num_enabled_extensions++] = Num_Extra_Instance_Extensions[i];
    }
    
    VkInstanceCreateFlags instance_create_flags = 0;
#if VD_PLATFORM_MACOS
    instance_create_flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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
            .flags = instance_create_flags,
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

#if VD_VULKAN_OBJECT_NAMES
    renderer->vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)
        vkGetInstanceProcAddr(
            renderer->instance,
            "vkSetDebugUtilsObjectNameEXT");

    assert(renderer->vkSetDebugUtilsObjectNameEXT);
#endif

// ----PICK PHYSICAL DEVICE-------------------------------------------------------------------------
    TracyCZoneN(Pick_Physical_Device, "Pick Physical Device", 1);

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
        int q_device_supports_swapchain             = 0;

        dynarray VkExtensionProperties *device_extensions = 0;
        array_init(device_extensions, VD_MM_FRAME_ALLOCATOR());
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

        VD_DBG_FMT(
            "Renderer",
            "\tExtensions: %{u32}",
            array_len(device_extensions));

        for (int j = 0; j < array_len(device_extensions); ++j) {
            VD_DBG_FMT(
                "Renderer",
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

            if (queue_surface_support) {

                VD_DBG_FMT(
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

    VD_DBG_FMT("Renderer", "Best device: %{i32}", best_device);
    VD_DBG_FMT("Renderer", "\tGraphics Queue: %{u32}", best_device_graphics_queue_family);
    VD_DBG_FMT("Renderer", "\tPresent Queue: %{u32}", best_device_present_queue_family);

    renderer->physical_device = physical_devices[best_device];

    TracyCZoneEnd(Pick_Physical_Device);

// ----CREATE LOGICAL DEVICE------------------------------------------------------------------------
    TracyCZoneN(Create_Logical_Device, "Create Logical Device", 1);

    static const char *create_logical_device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if VD_PLATFORM_MACOS
        "VK_KHR_portability_subset"
#endif
    };

    VD_VK_CHECK(vkCreateDevice(
        renderer->physical_device,
        & (VkDeviceCreateInfo) 
        {
            .sType                          = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount           = 2,
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
            .enabledExtensionCount          = VD_ARRAY_COUNT(create_logical_device_extensions),
            .ppEnabledExtensionNames        = create_logical_device_extensions,
        },
        0,
        &renderer->device));
    volkLoadDevice(renderer->device);

    renderer->graphics.queue_family_index = best_device_graphics_queue_family;
    renderer->presentation.queue_family_index = best_device_present_queue_family;

    vkGetDeviceQueue(
        renderer->device,
        best_device_graphics_queue_family,
        0,
        &renderer->graphics.queue);

    vkGetDeviceQueue(
        renderer->device,
        best_device_present_queue_family,
        0,
        &renderer->presentation.queue);

    TracyCZoneEnd(Create_Logical_Device);
// ----VMA------------------------------------------------------------------------------------------

    VD_VK_CHECK(vmaCreateAllocator(
        & (VmaAllocatorCreateInfo)
        {
            .device                     = renderer->device,
            .physicalDevice             = renderer->physical_device,
            .instance                   = renderer->instance,
            .flags                      = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .pVulkanFunctions = & (VmaVulkanFunctions) {
                .vkGetInstanceProcAddr                  = vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr                    = vkGetDeviceProcAddr,
                .vkAllocateMemory                       = vkAllocateMemory,
                .vkBindBufferMemory                     = vkBindBufferMemory,
                .vkBindImageMemory                      = vkBindImageMemory,
                .vkCreateBuffer                         = vkCreateBuffer,
                .vkCreateImage                          = vkCreateImage,
                .vkDestroyBuffer                        = vkDestroyBuffer,
                .vkDestroyImage                         = vkDestroyImage,
                .vkFlushMappedMemoryRanges              = vkFlushMappedMemoryRanges,
                .vkFreeMemory                           = vkFreeMemory,
                .vkGetBufferMemoryRequirements          = vkGetBufferMemoryRequirements,
                .vkGetImageMemoryRequirements           = vkGetImageMemoryRequirements,
                .vkGetPhysicalDeviceMemoryProperties    = vkGetPhysicalDeviceMemoryProperties,
                .vkGetPhysicalDeviceProperties          = vkGetPhysicalDeviceProperties,
                .vkInvalidateMappedMemoryRanges         = vkInvalidateMappedMemoryRanges,
                .vkMapMemory                            = vkMapMemory,
                .vkUnmapMemory                          = vkUnmapMemory,
                .vkCmdCopyBuffer                        = vkCmdCopyBuffer,
            },
        },
        &renderer->allocator));

// ----DELETION QUEUE-------------------------------------------------------------------------------
    vd_deletion_queue_init(
        &renderer->deletion_queue,
        & (VD_DeletionQueueInitInfo)
        {
            .allocator = VD_MM_GLOBAL_ALLOCATOR(),
            .renderer = renderer,
        });

// ----DEFAULT FORMATS------------------------------------------------------------------------------
    renderer->color_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
    renderer->depth_image_format = VK_FORMAT_D32_SFLOAT;

// ----SYSTEMS--------------------------------------------------------------------------------------
    vd_texture_system_init(&renderer->textures, & (VD_R_TextureSystemInitInfo) {
        .allocator = renderer->allocator,
        .device = renderer->device,
    });

    vd_r_geo_system_init(&renderer->geos, & (VD_R_GeoSystemInitInfo) {
        .allocator = renderer->allocator,
        .device = renderer->device,
    });

    vd_r_sshader_init(&renderer->sshader, & (SShaderInitInfo) {
        .device = renderer->device,
    });

    smat_init(&renderer->smat, & (SMatInitInfo) {
        .device = renderer->device,
        .allocator = renderer->allocator,
        .num_set0_bindings = 1,
        .set0_bindings = (BindingInfo[]) {
            (BindingInfo)
            {
                .type = BINDING_TYPE_STRUCT,
                .struct_size = sizeof(VD_R_SceneData),
            }
        },
        .color_format = renderer->color_image_format,
        .depth_format = renderer->depth_image_format,
        .default_push_constant = {
            .type = PUSH_CONSTANT_TYPE_DEFAULT,
            .size = sizeof(VD_R_SceneData),
        },
    });

// ----IMMEDIATE QUEUE------------------------------------------------------------------------------

    VD_VK_CHECK(vkCreateCommandPool(
        renderer->device,
        &(VkCommandPoolCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = renderer->graphics.queue_family_index,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        },
        0,
        &renderer->imm.command_pool));

    name_object(renderer, &(VkDebugUtilsObjectNameInfoEXT)
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_COMMAND_POOL,
        .objectHandle = (u64)renderer->imm.command_pool,
        .pObjectName = "Immediate Command Pool",
    });

    VD_VK_CHECK(vkAllocateCommandBuffers(
        renderer->device,
        & (VkCommandBufferAllocateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = renderer->imm.command_pool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        },
        &renderer->imm.command_buffer));

    name_object(renderer, &(VkDebugUtilsObjectNameInfoEXT)
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
        .objectHandle = (u64)renderer->imm.command_buffer,
        .pObjectName = "Immediate Command Buffer",
    });

    VD_VK_CHECK(vkCreateFence(
        renderer->device,
        & (VkFenceCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        },
        0,
        &renderer->imm.fence));

// ----DEFAULT MESHES-------------------------------------------------------------------------------
    {
        VD_R_Vertex vertices[4] =
        {
            {
                .position   = {0.5,-0.5, 0},
                .color      = {0,0,0,1},
            },
            {
                .position   = {0.5,0.5, 0},
                .color      = {0.5,0.5,0.5 ,1},
            },
            {
                .position   = {-0.5,-0.5, 0},
                .color      = {1,0, 0,1},
            },
            {
                .position   = {-0.5,0.5, 0},
                .color      = {0,1, 0,1},
            },
        };

        u32 indices[6] = {0,1,2, 2,1,3};

        renderer->meshes.quad = vd_renderer_create_mesh(renderer, & (VD_R_MeshCreateInfo){
            .num_vertices = VD_ARRAY_COUNT(vertices),
            .num_indices = VD_ARRAY_COUNT(indices),
        });

        vd_renderer_write_mesh(renderer, & (VD_R_MeshWriteInfo) {
            .mesh = renderer->meshes.quad,
            .vertices = vertices,
            .indices = indices,
            .num_indices = VD_ARRAY_COUNT(indices),
            .num_vertices = VD_ARRAY_COUNT(vertices),
        });
    }

    {
        VD_R_Vertex *vertices;
        int num_vertices;
        u32 *indices;
        int num_indices;
        vd_r_generate_sphere_data(
            &vertices,
            &num_vertices,
            &indices,
            &num_indices,
            0.5f,
            16,
            16,
            VD_MM_FRAME_ALLOCATOR());

        renderer->meshes.sphere = vd_renderer_create_mesh(renderer, & (VD_R_MeshCreateInfo){
            .num_vertices = num_vertices,
            .num_indices = num_indices,
        });

        vd_renderer_write_mesh(renderer, & (VD_R_MeshWriteInfo) {
            .mesh = renderer->meshes.sphere,
            .vertices = vertices,
            .indices = indices,
            .num_indices = num_indices,
            .num_vertices = num_vertices,
        });

    }

    {
        VD_R_Vertex *vertices;
        int num_vertices;
        u32 *indices;
        int num_indices;
        vec3 extents = {1.0f, 1.0f, 1.0f};
        vd_r_generate_cube_data(
            &vertices,
            &num_vertices,
            &indices,
            &num_indices,
            extents,
            VD_MM_FRAME_ALLOCATOR());

        renderer->meshes.cube = vd_renderer_create_mesh(renderer, & (VD_R_MeshCreateInfo){
            .num_vertices = num_vertices,
            .num_indices = num_indices,
        });

        vd_renderer_write_mesh(renderer, & (VD_R_MeshWriteInfo) {
            .mesh = renderer->meshes.cube,
            .vertices = vertices,
            .indices = indices,
            .num_indices = num_indices,
            .num_vertices = num_vertices,
        });

    }
// ----DEFAULT IMAGES-------------------------------------------------------------------------------
    {
        u32 black   = vd_pack_unorm_r8g8b8a8((float[4]) { 0.0f,  0.0f,  0.0f,  1.0f});
        u32 white   = vd_pack_unorm_r8g8b8a8((float[4]) { 1.0f,  1.0f,  1.0f,  1.0f});
        u32 grey    = vd_pack_unorm_r8g8b8a8((float[4]) { 0.66f, 0.66f, 0.66f, 1.0f});
        u32 magenta = vd_pack_unorm_r8g8b8a8((float[4]) { 1.0,   0.0f,  1.0f,  1.0f});

        renderer->images.white = vd_renderer_create_texture(
            renderer,
            & (VD_R_TextureCreateInfo)
            {
                .extent     = { 1, 1, 1 },
                .usage      = VK_IMAGE_USAGE_SAMPLED_BIT,
                .format     = VK_FORMAT_R8G8B8A8_UNORM,
                .mipmapping = {
                    .on = 0,
                },
            });

        vd_renderer_upload_texture_data(
            renderer,
            USE_HANDLE(renderer->images.white, VD_R_AllocatedImage),
            &white,
            sizeof(white));

        renderer->images.black = vd_renderer_create_texture(
            renderer,
            & (VD_R_TextureCreateInfo)
            {
                .extent     = { 1, 1, 1 },
                .usage      = VK_IMAGE_USAGE_SAMPLED_BIT,
                .format     = VK_FORMAT_R8G8B8A8_UNORM,
                .mipmapping = {
                    .on = 0,
                },
            });

        vd_renderer_upload_texture_data(
            renderer,
            USE_HANDLE(renderer->images.black, VD_R_AllocatedImage),
            &black,
            sizeof(black));

        size_t checkers_size;
        void *checkers = vd_r_generate_checkerboard(
            magenta,
            black,
            16,
            16,
            &checkers_size,
            VD_MM_FRAME_ALLOCATOR());

        renderer->images.checker_magenta = vd_renderer_create_texture(
            renderer,
            & (VD_R_TextureCreateInfo)
            {
                .extent     = { 16, 16, 1 },
                .usage      = VK_IMAGE_USAGE_SAMPLED_BIT,
                .format     = VK_FORMAT_R8G8B8A8_UNORM,
                .mipmapping = {
                    .on = 0,
                },
            });

        vd_renderer_upload_texture_data(
            renderer,
            USE_HANDLE(renderer->images.checker_magenta, VD_R_AllocatedImage),
            checkers,
            checkers_size);

    }

// ----DEFAULT PIPELINES----------------------------------------------------------------------------
    {
        TracyCZoneN(Create_Pipeline_Opaque, "Create Pipeline Opaque", 1);

        HandleOf(GPUShader) vertex, fragment;

        TracyCZoneN(Compile_Shaders, "Compile Shaders", 1);
        {
            vertex = vd_renderer_create_shader(renderer, & (GPUShaderCreateInfo) {
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .sourcecode = VD_PBROPAQUE_VERT,
                .sourcecode_len = strlen(VD_PBROPAQUE_VERT),
            });
        }

        {
            fragment = vd_renderer_create_shader(renderer, & (GPUShaderCreateInfo) {
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .sourcecode = VD_PBROPAQUE_FRAG,
                .sourcecode_len = strlen(VD_PBROPAQUE_FRAG),
            });
        }
        TracyCZoneEnd(Compile_Shaders);

        renderer->materials.pbropaque = smat_new_blueprint(&renderer->smat, & (MaterialBlueprint)
        {
            .blend.on = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .cull_face = VK_FRONT_FACE_CLOCKWISE,
            .cull_mode = VK_CULL_MODE_BACK_BIT,
            .depth_test = {
                .on = 1,
                .write = 1,
                .cmp_op = VK_COMPARE_OP_GREATER_OR_EQUAL,
            },
            .multisample.on = 0,
            .polygon_mode = VK_POLYGON_MODE_FILL,
            .num_shaders = 2,
            .shaders = {
                vertex,
                fragment,
            },
            .pass = VD_PASS_OPAQUE,
            .num_properties = 1,
            .properties = (MaterialProperty[])
            {
                (MaterialProperty)
                {
                    .binding.type = BINDING_TYPE_SAMPLER2D,
                    .sampler2d = renderer->images.checker_magenta,
                },
            },
        });

        DROP_HANDLE(vertex);
        DROP_HANDLE(fragment);

        TracyCZoneEnd(Create_Pipeline_Opaque);
    }

    array_init(renderer->render_object_list, vd_memory_get_system_allocator());

// ----CVARS----------------------------------------------------------------------------------------
    VD_CVS_SET_INT("r.inflight-frame-count", 2);

    {
        ecs_entity_t ent = ecs_entity(renderer->world, { .name = "sphere" });
        ecs_set(renderer->world, ent, StaticMeshComponent, {
            .mesh = renderer->meshes.sphere,
            .material = smat_new_from_blueprint(&renderer->smat, renderer->materials.pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        glm_translate_x(matrix, -1.0f);

        ecs_add(renderer->world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            renderer->world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

    }

    {
        ecs_entity_t ent = ecs_entity(renderer->world, { .name = "cube" });
        ecs_set(renderer->world, ent, StaticMeshComponent, {
            .mesh = renderer->meshes.cube,
            .material = smat_new_from_blueprint(&renderer->smat, renderer->materials.pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, 1.0f);
        glm_rotate(matrix, glm_rad(45.0f), rotation_vector);

        ecs_add(renderer->world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            renderer->world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

    }

    {
        ecs_entity_t ent = ecs_entity(renderer->world, { .name = "cube 1" });
        ecs_set(renderer->world, ent, StaticMeshComponent, {
            .mesh = renderer->meshes.cube,
            .material = smat_new_from_blueprint(&renderer->smat, renderer->materials.pbropaque),
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, -1.1f);
        glm_rotate(matrix, glm_rad(-45.0f), rotation_vector);

        ecs_add(renderer->world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            renderer->world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

    }
    {
        ecs_entity_t ent = ecs_entity(renderer->world, { .name = "cube 2" });
        HandleOf(GPUMaterial) mat = smat_new_from_blueprint(
            &renderer->smat,
            renderer->materials.pbropaque);

        USE_HANDLE(mat, GPUMaterial)->properties[0].sampler2d = renderer->images.white;
        ecs_set(renderer->world, ent, StaticMeshComponent, {
            .mesh = renderer->meshes.cube,
            .material = mat,
        });

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        vec3 rotation_vector = { 0.7, 1, 0.1 };
        glm_normalize(rotation_vector);
        glm_translate_y(matrix, -1.1f);
        glm_translate_x(matrix, -1);
        glm_rotate(matrix, glm_rad(25.0f), rotation_vector);

        ecs_add(renderer->world, ent, WorldTransformComponent);
        WorldTransformComponent *c = ecs_get_mut(
            renderer->world,
            ent,
            WorldTransformComponent);
        glm_mat4_copy(matrix, c->world);

    }
    return 0;
}

static void rebuild_frame_data(
    const char *name,
    VD_Renderer *renderer,
    ecs_entity_t entity,
    dynarray VD_RendererFrameData **out_frame_data)
{
    VD_RendererFrameData *frame_data = *out_frame_data;
    i32 num_inflight_frames;
    VD_CVS_GET_INT("r.inflight-frame-count", &num_inflight_frames);

    array_clear(frame_data);
    array_addn(frame_data, num_inflight_frames);

    for (int i = 0; i < array_len(frame_data); ++i) {

        VD_VK_CHECK(vkCreateCommandPool(
            renderer->device,
            & (VkCommandPoolCreateInfo)
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex   = renderer->graphics.queue_family_index,
                .pNext              = 0,
            },
            0,
            &frame_data[i].command_pool));

        VD_VK_CHECK(vkAllocateCommandBuffers(
            renderer->device,
            & (VkCommandBufferAllocateInfo)
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = frame_data[i].command_pool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
                .pNext              = 0,
            },
            &frame_data[i].command_buffer));

        VD_VK_CHECK(vkCreateFence(
            renderer->device,
            &(VkFenceCreateInfo){
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = 0,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            },
            0,
            &frame_data[i].fnc_render_complete));

        VD_VK_CHECK(vkCreateSemaphore(
            renderer->device,
            &(VkSemaphoreCreateInfo){
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = 0,
                .flags = 0,
            },
            0,
            &frame_data[i].sem_image_available));

        VD_VK_CHECK(vkCreateSemaphore(
            renderer->device,
            &(VkSemaphoreCreateInfo){
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = 0,
                .flags = 0,
            },
            0,
            &frame_data[i].sem_present_image));

        vd_descriptor_allocator_init(
            &frame_data[i].descriptor_allocator,
            & (VD_DescriptorAllocatorInitInfo)
            {
                .device = renderer->device,
                .initial_sets = 1000,
                .ratios = (VD_DescriptorPoolSizeRatio[])
                {
                    (VD_DescriptorPoolSizeRatio) { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,            3 },
                    (VD_DescriptorPoolSizeRatio) { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,           3 },
                    (VD_DescriptorPoolSizeRatio) { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,           3 },
                    (VD_DescriptorPoolSizeRatio) { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,   4 },
                },
                .num_ratios = 4,
            });

        vd_deletion_queue_init(
            &frame_data[i].deletion_queue,
            & (VD_DeletionQueueInitInfo)
            {
                .allocator = VD_MM_GLOBAL_ALLOCATOR(),
                .renderer = renderer,
            });
    }
    *out_frame_data = frame_data;
}

static void create_swapchain_image_views_and_framebuffers(
    const char                      *name,
    ecs_entity_t                    entity,
    VD_Renderer                     *renderer,
    VkSurfaceKHR                    surface,
    VkExtent2D                      extent,
    VkSwapchainKHR                  *out_swapchain,
    VkFormat                        *out_format,
    dynarray VkImage                **out_images,
    dynarray VkImageView            **out_image_views,
    dynarray VD_RendererFrameData   **out_frame_data,
    VD_R_AllocatedImage             *out_color_image,
    VD_R_AllocatedImage             *out_depth_image)
{
    u32                 image_count;
    VkSurfaceFormatKHR  best_surface_format = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
    int                 best_present_mode = -1;
    
    VkSwapchainKHR                  swapchain;
    dynarray VkImage                *images = 0;
    dynarray VkImageView            *image_views = 0;
    dynarray VD_RendererFrameData   *frame_data = 0;

    VD_LOG_FMT("Renderer", "Creating swapchain for window with name: %{cstr}", name);

// ----SURFACE CAPABILITIES-------------------------------------------------------------------------
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VD_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        renderer->physical_device,
        surface,
        &surface_capabilities));

    image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 &&
        image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

// ----SURFACE FORMAT-------------------------------------------------------------------------------
    dynarray VkSurfaceFormatKHR *surface_formats = 0;
    array_init(surface_formats, VD_MM_FRAME_ALLOCATOR());
    {
        u32 surface_format_count;
        VD_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            renderer->physical_device,
            surface,
            &surface_format_count,
            0));

        array_addn(surface_formats, surface_format_count);

        VD_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            renderer->physical_device,
            surface,
            &surface_format_count,
            surface_formats));
    }

    for (int i = 0; i < array_len(surface_formats); ++i)
    {
        VkSurfaceFormatKHR q_surface_format = surface_formats[i];

        for (int j = 0; j < ARRAY_COUNT(Preferred_Surface_Formats); ++j)
        {
            if (q_surface_format.format == Preferred_Surface_Formats[j].format &&
                q_surface_format.colorSpace == Preferred_Surface_Formats[j].colorSpace)
            {
                best_surface_format = q_surface_format;
                break;
            }
        }
    }

    if (best_surface_format.format == VK_FORMAT_UNDEFINED) {
        VD_LOG_FMT(
            "Renderer",
            "No preferred surface format found for window with name: %{cstr}", name);
        abort();
    }

    VD_LOG_FMT(
        "Renderer",
        "Chosen Surface Format:%{cstr} %{cstr}",
        vkformat_to_string(best_surface_format.format),
        vkcolorspacekhr_to_string(best_surface_format.colorSpace));

// ----PRESENT MODE---------------------------------------------------------------------------------

    dynarray VkPresentModeKHR *surface_present_modes = 0;
    array_init(surface_present_modes , VD_MM_FRAME_ALLOCATOR());
    {
        u32 num_surface_present_modes;
        VD_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            renderer->physical_device,
            surface,
            &num_surface_present_modes,
            0));

        array_addn(surface_present_modes, num_surface_present_modes);
        
        VD_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
            renderer->physical_device,
            surface,
            &num_surface_present_modes,
            surface_present_modes));
    }


    for (int i = 0; i < array_len(surface_present_modes); ++i)
    {
        VD_LOG_FMT(
            "Renderer",
            "Present Mode[%{i32}]: %{cstr}",
            i,
            vkpresentmodekhr_to_string(surface_present_modes[i]));

        for (int j = 0; j < ARRAY_COUNT(Preferred_Present_Modes); ++j)
        {
            if (surface_present_modes[i] == Preferred_Present_Modes[j])
            {
                best_present_mode = i;
                break;
            }
        }
    }

    if (best_present_mode == -1) {
        VD_LOG_FMT(
            "Renderer",
            "No preferred present mode found for window with name: %{cstr}", name);
        abort();
    }

    VD_LOG_FMT(
        "Renderer",
        "Chosen Present Mode: %{cstr}",
        vkpresentmodekhr_to_string(surface_present_modes[best_present_mode]));

    VD_VK_CHECK(vkCreateSwapchainKHR(
        renderer->device,
        &(VkSwapchainCreateInfoKHR){
            .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface                = surface,
            .minImageCount          = image_count,
            .imageFormat            = best_surface_format.format,
            .imageColorSpace        = best_surface_format.colorSpace,
            .imageExtent            = extent,
            .imageArrayLayers       = 1,
            .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .preTransform           = surface_capabilities.currentTransform,
            .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .imageSharingMode       = renderer->graphics.queue_family_index == renderer->presentation.queue_family_index
                ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = renderer->graphics.queue_family_index == renderer->presentation.queue_family_index 
                ? 0 : 2,
            .pQueueFamilyIndices    = renderer->graphics.queue_family_index == renderer->presentation.queue_family_index
                ? 0 : (u32[]) 
                {
                    renderer->graphics.queue_family_index, renderer->presentation.queue_family_index
                },
            .presentMode            = surface_present_modes[best_present_mode],
            .clipped                = VK_TRUE,
            .oldSwapchain           = VK_NULL_HANDLE,
        },
        0,
        &swapchain));


    VD_Allocator entity_allocator = VD_MM_ENTITY_ALLOCATOR(entity);
// ----SWAPCHAIN IMAGES-----------------------------------------------------------------------------
    array_init(images, &entity_allocator);
    {
        u32 num_swapchain_images;
        VD_VK_CHECK(vkGetSwapchainImagesKHR(
            renderer->device,
            swapchain,
            &num_swapchain_images,
            0));

        array_addn(images, num_swapchain_images);

        VD_VK_CHECK(vkGetSwapchainImagesKHR(
            renderer->device,
            swapchain,
            &num_swapchain_images,
            images));

        for (int i = 0; i < array_len(images); ++i) {
            name_object(
                renderer,
                & (VkDebugUtilsObjectNameInfoEXT)
                {
                    .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                    .objectType         = VK_OBJECT_TYPE_IMAGE,
                    .objectHandle       = (u64)images[i],
                    .pObjectName        = vd_snfmt_a(
                                            VD_MM_FRAME_ALLOCATOR(),
                                            "Swapchain Image[%{i32}]%{null}",
                                            i).data,
                });
        }
    }
    
// ----SWAPCHAIN IMAGE VIEWS------------------------------------------------------------------------
    array_init(image_views, &entity_allocator);
    {
        array_addn(image_views, array_len(images));

        for (int i = 0; i < array_len(image_views); ++i) {
            VD_VK_CHECK(vkCreateImageView(
                renderer->device,
                & (VkImageViewCreateInfo) {
                    .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image              = images[i],
                    .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                    .format             = best_surface_format.format,
                    .components         = {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange   = {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                },
                0,
                &image_views[i]));
        }
    }

// ----FRAME DATA-----------------------------------------------------------------------------------
    if (*out_frame_data == 0) {
        array_init(frame_data, &entity_allocator);
        rebuild_frame_data(name, renderer, entity, &frame_data);
    } else {
        frame_data = *out_frame_data;
    }
    
// ----COLOR IMAGE----------------------------------------------------------------------------------
    out_color_image->format = renderer->color_image_format;
    VD_VK_CHECK(vmaCreateImage(
        renderer->allocator,
        & (VkImageCreateInfo)
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType              = VK_IMAGE_TYPE_2D,
            .mipLevels              = 1,
            .arrayLayers            = 1,
            .samples                = VK_SAMPLE_COUNT_1_BIT,
            .tiling                 = VK_IMAGE_TILING_OPTIMAL,
            .format                 = out_color_image->format,
            .extent                 = {extent.width, extent.height, 1},
            .usage                  = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_STORAGE_BIT,                            
        },
        & (VmaAllocationCreateInfo)
        {
            .requiredFlags          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .usage                  = VMA_MEMORY_USAGE_GPU_ONLY,
        },
        &out_color_image->image,
        &out_color_image->allocation,
        0));

    name_object(
        renderer,
        & (VkDebugUtilsObjectNameInfoEXT)
        {
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType         = VK_OBJECT_TYPE_IMAGE,
            .objectHandle       = (u64)out_color_image->image,
            .pObjectName        = "Color Image",
        });

    VD_VK_CHECK(vkCreateImageView(
        renderer->device,
        & (VkImageViewCreateInfo)
        {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format             = out_color_image->format,
            .image              = out_color_image->image,
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange   =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        },
        0,
        &out_color_image->view));

// ----DEPTH IMAGE----------------------------------------------------------------------------------
    out_depth_image->format = renderer->depth_image_format;
    VD_VK_CHECK(vmaCreateImage(
        renderer->allocator,
        & (VkImageCreateInfo)
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType              = VK_IMAGE_TYPE_2D,
            .mipLevels              = 1,
            .arrayLayers            = 1,
            .samples                = VK_SAMPLE_COUNT_1_BIT,
            .tiling                 = VK_IMAGE_TILING_OPTIMAL,
            .format                 = out_depth_image->format,
            .extent                 = {extent.width, extent.height, 1},
            .usage                  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        },
        & (VmaAllocationCreateInfo)
        {
            .requiredFlags          = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .usage                  = VMA_MEMORY_USAGE_GPU_ONLY,
        },
        &out_depth_image->image,
        &out_depth_image->allocation,
        0));

    name_object(
        renderer,
        & (VkDebugUtilsObjectNameInfoEXT)
        {
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType         = VK_OBJECT_TYPE_IMAGE,
            .objectHandle       = (u64)out_depth_image->image,
            .pObjectName        = "Depth Image",
        });

    VD_VK_CHECK(vkCreateImageView(
        renderer->device,
        & (VkImageViewCreateInfo)
        {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format             = out_depth_image->format,
            .image              = out_depth_image->image,
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange   =
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        },
        0,
        &out_depth_image->view));

    *out_swapchain      = swapchain;
    *out_format         = best_surface_format.format;
    *out_images         = images;
    *out_image_views    = image_views;
    *out_frame_data     = frame_data;
}

void on_window_component_immediate_destroy(ecs_entity_t entity, void *usrdata);

void RendererOnWindowComponentSet(ecs_iter_t *it)
{
    const Application *app = ecs_singleton_get(it->world, Application);
    VD_Renderer *renderer = vd_instance_get_renderer(app->instance);

    WindowComponent *w = ecs_field(it, WindowComponent, 0);

    
    for (int i = 0; i < it->count; ++i) {
        if (w[i].create_surface == 0) {
            continue;
        }

        if (ecs_get(it->world, it->entities[i], WindowSurfaceComponent)) {
            continue;
        }

        const Size2D *window_size = ecs_get(it->world, it->entities[i], Size2D);
        VkSurfaceKHR surface = w[i].create_surface(&w[i], renderer->instance);

        vkDeviceWaitIdle(renderer->device);

        VkSwapchainKHR                  swapchain;
        VkFormat                        surface_format;
        dynarray VkImage                *images;
        dynarray VkImageView            *image_views;
        dynarray VD_RendererFrameData   *frame_data = 0;
        VD_R_AllocatedImage             color_image;
        VD_R_AllocatedImage             depth_image;
        create_swapchain_image_views_and_framebuffers(
            ecs_get_name(it->world, it->entities[i]),
            it->entities[i],
            renderer,
            surface,
            (VkExtent2D) { window_size->x, window_size->y },
            &swapchain,
            &surface_format,
            &images,
            &image_views,
            &frame_data,
            &color_image,
            &depth_image);

        ecs_set(it->world, it->entities[i], WindowSurfaceComponent, {
            .swapchain = swapchain,
            .surface = surface,
            .surface_format = surface_format,
            .images = images,
            .image_views = image_views,
            .extent = { window_size->x, window_size->y },
            .frame_data = frame_data,
            .current_frame = 0,
            .color_image = color_image,
            .depth_image = depth_image,
        });

        VD_CALLBACK_SET(w[i].on_immediate_destroy, on_window_component_immediate_destroy, renderer);
        ecs_modified(it->world, it->entities[i], WindowComponent);
    }
}

void on_window_component_immediate_destroy(ecs_entity_t entity, void *usrdata)
{
    VD_Renderer *renderer = (VD_Renderer*)usrdata;
    WindowSurfaceComponent *ws = (WindowSurfaceComponent*)
        ecs_get(renderer->world, entity, WindowSurfaceComponent);

    vkDeviceWaitIdle(renderer->device);

    vkDestroyImageView(renderer->device, ws->color_image.view, 0);
    vkDestroyImageView(renderer->device, ws->depth_image.view, 0);
    vmaDestroyImage(renderer->allocator, ws->color_image.image, ws->color_image.allocation);
    vmaDestroyImage(renderer->allocator, ws->depth_image.image, ws->depth_image.allocation);

    for (int i = 0; i < array_len(ws->frame_data); ++i) {
        vd_deletion_queue_flush(&ws->frame_data[i].deletion_queue);
        vkDestroyFence(renderer->device, ws->frame_data[i].fnc_render_complete, 0);
        vkDestroySemaphore(renderer->device, ws->frame_data[i].sem_image_available, 0);
        vkDestroySemaphore(renderer->device, ws->frame_data[i].sem_present_image, 0);
        vkDestroyCommandPool(renderer->device, ws->frame_data[i].command_pool, 0);
        vd_descriptor_allocator_deinit(&ws->frame_data[i].descriptor_allocator);
    }

    for (int i = 0; i < array_len(ws->image_views); ++i) {
        vkDestroyImageView(renderer->device, ws->image_views[i], 0);
    }

    vkDestroySwapchainKHR(renderer->device, ws->swapchain, 0);
    vkDestroySurfaceKHR(renderer->instance, ws->surface, 0);
}

int vd_renderer_deinit(VD_Renderer *renderer)
{
    vd_texture_system_deinit(&renderer->textures);
    vd_r_geo_system_deinit(&renderer->geos);
    vd_r_sshader_deinit(&renderer->sshader);
    vkDestroyCommandPool(renderer->device, renderer->imm.command_pool, 0);
    vkDestroyFence(renderer->device, renderer->imm.fence, 0);

    vd_deletion_queue_flush(&renderer->deletion_queue);
    vmaDestroyAllocator(renderer->allocator);
    vkDestroyDevice(renderer->device, 0);
#if VD_VALIDATION_LAYERS
    renderer->vkDestroyDebugUtilsMessengerEXT(renderer->instance, renderer->debug_messenger, 0);
#endif
    vkDestroyInstance(renderer->instance, 0);
    return 0;
}

static void render_window_surface(
    VD_Renderer *renderer,
    WindowSurfaceComponent *ws)
{
    VD_RendererFrameData *frame_data = 
        &ws->frame_data[ws->current_frame % array_len(ws->frame_data)];
    ws->current_frame++;

    VD_VK_CHECK(vkWaitForFences(
        renderer->device,
        1,
        &frame_data->fnc_render_complete,
        VK_TRUE,
        1000000000));

    VD_VK_CHECK(vkResetFences(
        renderer->device,
        1,
        &frame_data->fnc_render_complete));

    smat_begin_frame(&renderer->smat, &frame_data->descriptor_allocator);
    vd_deletion_queue_flush(&frame_data->deletion_queue);

    u32 swapchain_image_idx;
    VD_VK_CHECK(vkAcquireNextImageKHR(
        renderer->device,
        ws->swapchain,
        1000000000,
        frame_data->sem_image_available,
        0,
        &swapchain_image_idx));

    VkCommandBuffer cmd = frame_data->command_buffer;
    vkResetCommandBuffer(cmd, 0);

    VD_VK_CHECK(vkBeginCommandBuffer(
        cmd,
        & (VkCommandBufferBeginInfo)
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pNext = 0,
        }));

    vd_vk_image_transition(
        cmd,
        ws->color_image.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL);

    VkImageSubresourceRange range = vd_vk_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(
        cmd,
        ws->color_image.image,
        VK_IMAGE_LAYOUT_GENERAL,
        & (VkClearColorValue) {{  0.2f, 0.2f, 0.2f, 1.0f }},
        1,
        &range);

    vd_vk_image_transition(
        cmd,
        ws->color_image.image,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vd_vk_image_transition(
        cmd,
        ws->depth_image.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(
        cmd,
        & (VkRenderingInfo)
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { .offset = { 0, 0 }, .extent = ws->extent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = (VkRenderingAttachmentInfo[])
            {
                {
                    .sType          = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView      = ws->color_image.view,
                    .imageLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                },
            },
            .pDepthAttachment = & (VkRenderingAttachmentInfo)
            {
                .sType          = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView      = ws->depth_image.view,
                .imageLayout    = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue.depthStencil.depth = 0.0f,
            },
        });

    vkCmdSetViewport(
        cmd,
        0,
        1,
        & (VkViewport)
        {
            .x = 0,
            .y = 0,
            .width = ws->extent.width,
            .height = ws->extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        });

    vkCmdSetScissor(
        cmd,
        0,
        1,
        & (VkRect2D)
        {
            .offset = { 0, 0 },
            .extent = ws->extent,
        });

    float aspect_ratio = (float)ws->extent.width / (float)ws->extent.height;
    mat4 projmatrix;
    vd_r_perspective(projmatrix, glm_rad(40.0f), aspect_ratio, 0.01f, 100.0f);

    static float dt = 0.0f;
    dt += 0.01f;
    vec3 up = {0, 1, 0};


    mat4 viewmatrix = GLM_MAT4_IDENTITY_INIT;
    float radius = 5.0f;
    vec3 eye = {sinf(glm_rad(dt) * 2) * radius, sinf(glm_rad(dt) * 1.5) * radius, cosf(glm_rad(dt) * 2) * radius};
    glm_lookat(eye, GLM_VEC3_ZERO, up, viewmatrix);

    VD_R_SceneData scene_data;
    glm_mat4_copy(projmatrix, scene_data.proj);
    glm_mat4_copy(viewmatrix, scene_data.view);

    vec3 sun_direction = {0.0f, 0.0f, -1.0f};
    glm_vec3_copy(sun_direction, scene_data.sun_direction);
    glm_normalize(scene_data.sun_direction);

    for (int i = 0; i < array_len(renderer->render_object_list); ++i) {
        HandleOf(GPUMaterial) material = renderer->render_object_list[i].material;
        GPUMaterial *materialptr = USE_HANDLE(material, GPUMaterial);
        HandleOf(GPUMaterialBlueprint) blueprint = materialptr->blueprint;
        GPUMaterialBlueprint *blueprintptr = USE_HANDLE(materialptr->blueprint, GPUMaterialBlueprint);

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blueprintptr->pipeline);

        GPUMaterialInstance instance = smat_prep(
            &renderer->smat,
            & (MaterialWriteInfo)
            {
                .material = material,
                .num_properties = 1,
                .properties = (MaterialProperty[])
                {
                    (MaterialProperty)
                    {
                        .binding.type = BINDING_TYPE_STRUCT,
                        .binding.struct_size = sizeof(scene_data),
                        .pstruct = &scene_data,
                    },
                },
            });

        VD_R_GPUMesh *mesh_to_draw = USE_HANDLE(renderer->render_object_list[i].mesh, VD_R_GPUMesh);
        VD_R_GPUPushConstants constants = {
            .vertex_buffer = mesh_to_draw->vertex_buffer_address,
            .obj = GLM_MAT4_IDENTITY_INIT,
        };

        glm_mat4_copy(renderer->render_object_list[i].transform, constants.obj);

        vkCmdPushConstants(
            cmd,
            blueprintptr->layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(constants),
            &constants);

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            blueprintptr->layout,
            0,
            2,
            (VkDescriptorSet[])
            {
                instance.default_set,
                instance.property_set,
            },
            0,
            0);

        vkCmdBindIndexBuffer(cmd, mesh_to_draw->index.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, mesh_to_draw->num_indices, 1, 0, 0, 0);
    }
    vkCmdEndRendering(cmd);

    smat_end_frame(&renderer->smat);

    vd_vk_image_transition(
        cmd,
        ws->color_image.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vd_vk_image_transition(
        cmd,
        ws->images[swapchain_image_idx],
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vd_vk_image_copy(
        cmd,
        ws->color_image.image,
        ws->images[swapchain_image_idx],
        (VkExtent2D) { ws->extent.width, ws->extent.height },
        (VkExtent2D) { ws->extent.width, ws->extent.height });

    vd_vk_image_transition(
        cmd,
        ws->images[swapchain_image_idx],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VD_VK_CHECK(vkEndCommandBuffer(cmd));

    VD_VK_CHECK(vkQueueSubmit2(
        renderer->graphics.queue,
        1,
        & (VkSubmitInfo2)
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = & (VkSemaphoreSubmitInfoKHR)
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                .semaphore = frame_data->sem_image_available,
                .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                .value = 1,
            },
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = & (VkSemaphoreSubmitInfoKHR)
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                .semaphore = frame_data->sem_present_image,
                .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .value = 1,
            },
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = & (VkCommandBufferSubmitInfo)
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = cmd,
            }
        },
        frame_data->fnc_render_complete));

    vkQueuePresentKHR(
        renderer->presentation.queue,
        & (VkPresentInfoKHR)
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pSwapchains = &ws->swapchain,
            .swapchainCount = 1,
            .pImageIndices = &swapchain_image_idx,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame_data->sem_present_image,
        });

}

VD_R_AllocatedBuffer vd_renderer_create_buffer(
    VD_Renderer *renderer,
    size_t size,
    VkBufferUsageFlags flags,
    VmaMemoryUsage usage)
{
    VD_R_AllocatedBuffer result;

    VD_VK_CHECK(vmaCreateBuffer(
        renderer->allocator,
        & (VkBufferCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = flags,
            .size = size,
        },
        & (VmaAllocationCreateInfo)
        {
            .usage = usage,
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        },
        &result.buffer,
        &result.allocation,
        &result.info));

    return result;
}

void *vd_renderer_map_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer)
{
    void *data;
    vmaMapMemory(renderer->allocator, buffer->allocation, &data);
    return data;
}

void vd_renderer_unmap_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer)
{
    vmaUnmapMemory(renderer->allocator, buffer->allocation);
}

VkDevice vd_renderer_get_device(VD_Renderer *renderer)
{
    return renderer->device;
}

void vd_renderer_destroy_buffer(VD_Renderer *renderer, VD_R_AllocatedBuffer *buffer)
{
    vmaDestroyBuffer(renderer->allocator, buffer->buffer, buffer->allocation);
}

VD_Handle vd_renderer_create_texture(
    VD_Renderer *renderer,
    VD_R_TextureCreateInfo *info)
{
    return vd_texture_system_new(&renderer->textures, info);
}

void vd_renderer_upload_texture_data(
    VD_Renderer *renderer,
    VD_R_AllocatedImage *image,
    void *data,
    size_t size)
{
    size_t data_size = image->extent.width * image->extent.height * image->extent.depth * 4;

    if (data_size != size)
    {
        VD_LOG("Renderer", "vd_renderer_upload_texture_data(): size != data_size!");
    }

    VD_R_AllocatedBuffer staging = vd_renderer_create_buffer(
        renderer,
        data_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    void *staging_ptr = vd_renderer_map_buffer(renderer, &staging);
    memcpy(staging_ptr, data, data_size);
    vd_renderer_unmap_buffer(renderer, &staging);

    VkCommandBuffer cmd = vd_renderer_imm_begin(renderer);
    {
        vd_vk_image_transition(
            cmd,
            image->image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyBufferToImage(
            cmd,
            staging.buffer,
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            & (VkBufferImageCopy)
            {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                },
                .imageExtent = image->extent,
            });

        vd_vk_image_transition(
            cmd,
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    }
    vd_renderer_imm_end(renderer);

    vd_renderer_destroy_buffer(renderer, &staging);
}

void vd_renderer_destroy_texture(
    VD_Renderer *renderer,
    VD_R_AllocatedImage *image)
{
    vkDestroyImageView(renderer->device, image->view, 0);
    vmaDestroyImage(renderer->allocator, image->image, image->allocation);
}

VD_R_GPUMesh vd_renderer_upload_mesh(
    VD_Renderer *renderer,
    u32 *indices,
    size_t num_indices,
    VD_R_Vertex *vertices,
    size_t num_vertices)
{
    size_t bytes_indices = sizeof(*indices) * num_indices;
    size_t bytes_vertices = sizeof(*vertices) * num_vertices;

    VD_R_GPUMesh result;

    result.index = vd_renderer_create_buffer(
        renderer,
        bytes_indices,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    result.vertex = vd_renderer_create_buffer(
        renderer,
        bytes_vertices,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    result.vertex_buffer_address = vkGetBufferDeviceAddress(
        renderer->device,
        & (VkBufferDeviceAddressInfo)
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = result.vertex.buffer,
        });


    VD_R_AllocatedBuffer staging_buffer = vd_renderer_create_buffer(
        renderer,
        bytes_indices + bytes_vertices,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer->allocator, staging_buffer.allocation, &data);
    memcpy(data, vertices, bytes_vertices);
    memcpy((char*)data + bytes_vertices, indices, bytes_indices);
    vmaUnmapMemory(renderer->allocator, staging_buffer.allocation);

    VkCommandBuffer cmd = vd_renderer_imm_begin(renderer);

    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        result.vertex.buffer,
        1,
        & (VkBufferCopy) { .srcOffset = 0, .dstOffset = 0, .size = bytes_vertices });


    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        result.index.buffer,
        1,
        &(VkBufferCopy) {.srcOffset = bytes_vertices, .dstOffset = 0, .size = bytes_indices });

    vd_renderer_imm_end(renderer);

    vd_renderer_destroy_buffer(renderer, &staging_buffer);

    return result;
}

VkCommandBuffer vd_renderer_imm_begin(VD_Renderer *renderer)
{
    VD_VK_CHECK(vkResetFences(renderer->device, 1, &renderer->imm.fence));
    VD_VK_CHECK(vkResetCommandBuffer(renderer->imm.command_buffer, 0));

    VD_VK_CHECK(vkBeginCommandBuffer(
        renderer->imm.command_buffer,
        & (VkCommandBufferBeginInfo)
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        }));
    
    return renderer->imm.command_buffer;
}

void vd_renderer_imm_end(VD_Renderer *renderer)
{
    vkEndCommandBuffer(renderer->imm.command_buffer);

    VD_VK_CHECK(vkQueueSubmit2(
        renderer->graphics.queue,
        1,
        &(VkSubmitInfo2)
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = & (VkCommandBufferSubmitInfo)
            {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = renderer->imm.command_buffer,
            },
        },
        renderer->imm.fence));

    VD_VK_CHECK(vkWaitForFences(renderer->device, 1, &renderer->imm.fence, 1, 99999999));
}

HandleOf(VD_R_GPUMesh) vd_renderer_create_mesh(
    VD_Renderer *renderer,
    VD_R_MeshCreateInfo *info)
{
    return vd_r_geo_system_new(&renderer->geos, info);
}

void vd_renderer_write_mesh(
    VD_Renderer *renderer,
    VD_R_MeshWriteInfo *info)
{
    size_t bytes_indices = sizeof(*info->indices) * info->num_indices;
    size_t bytes_vertices = sizeof(*info->vertices) * info->num_vertices;

    VD_R_GPUMesh *mesh = USE_HANDLE(info->mesh, VD_R_GPUMesh);

    VD_R_AllocatedBuffer staging_buffer = vd_renderer_create_buffer(
        renderer,
        bytes_indices + bytes_vertices,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer->allocator, staging_buffer.allocation, &data);
    memcpy(data, info->vertices, bytes_vertices);
    memcpy((char*)data + bytes_vertices, info->indices, bytes_indices);
    vmaUnmapMemory(renderer->allocator, staging_buffer.allocation);

    if (bytes_vertices > mesh->vertex.info.size || bytes_indices > mesh->index.info.size) {
        sgeo_resize(&renderer->geos, info->mesh, & (VD_R_MeshCreateInfo) {
            .num_vertices = info->num_vertices,
            .num_indices = info->num_indices,
        });
    }

    mesh->num_indices = info->num_indices;
    mesh->num_vertices = info->num_vertices;

    VkCommandBuffer cmd = vd_renderer_imm_begin(renderer);

    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        mesh->vertex.buffer,
        1,
        & (VkBufferCopy) { .srcOffset = 0, .dstOffset = 0, .size = bytes_vertices });

    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        mesh->index.buffer,
        1,
        &(VkBufferCopy) {.srcOffset = bytes_vertices, .dstOffset = 0, .size = bytes_indices });

    vd_renderer_imm_end(renderer);

    vd_renderer_destroy_buffer(renderer, &staging_buffer);
}

HandleOf(GPUShader) vd_renderer_create_shader(
    VD_Renderer *renderer,
    GPUShaderCreateInfo *info)
{
    return vd_r_sshader_new(&renderer->sshader, info);
}

HandleOf(GPUMaterialBlueprint) vd_renderer_create_material_blueprint(
    VD_Renderer *renderer,
    MaterialBlueprint *blueprint)
{
    return smat_new_blueprint(&renderer->smat, blueprint);
}

HandleOf(GPUMaterial) vd_renderer_create_material(
    VD_Renderer *renderer,
    HandleOf(GPUMaterialBlueprint) blueprint)
{
    return smat_new_from_blueprint(&renderer->smat, blueprint);
}

HandleOf(GPUMaterial) vd_renderer_get_default_material(VD_Renderer *renderer)
{
    return renderer->materials.pbropaque;
}

void vd_renderer_push_render_object(VD_Renderer *renderer, RenderObject *render_object)
{
    array_add(renderer->render_object_list, *render_object);
}

static void vd_shdc_log_error(const char *what, const char *msg, const char *extmsg)
{
    VD_LOG_FMT("SHDC", "%{cstr}: %{cstr} %{cstr}", what, msg, extmsg);
}


void RendererRenderToWindowSurfaceComponents(ecs_iter_t *it)
{
    const Application *app = ecs_singleton_get(it->world, Application);
    VD_Renderer *renderer = vd_instance_get_renderer(app->instance);
    WindowSurfaceComponent *ws = ecs_field(it, WindowSurfaceComponent, 0);

    for (int i = 0; i < it->count; ++i) {
        render_window_surface(renderer, &ws[i]);
    }

    array_clear(renderer->render_object_list);
}

void RendererCheckWindowComponentSizeChange(ecs_iter_t *it)
{
    const Application *app = ecs_singleton_get(it->world, Application);
    VD_Renderer *renderer = vd_instance_get_renderer(app->instance);
    WindowComponent *w  = ecs_field(it, WindowComponent, 0);
    Size2D *sizes       = ecs_field(it, Size2D,          1);

    for (int i = 0; i < it->count; ++i) {
        if (!w[i].just_resized) {
            continue;
        }

        WindowSurfaceComponent *ws =
            (WindowSurfaceComponent*)ecs_get(it->world, it->entities[i], WindowSurfaceComponent);

        VD_LOG_FMT(
            "Renderer",
            "Window %{cstr} just resized!",
            ecs_get_name(it->world, it->entities[i]));
        vkDeviceWaitIdle(renderer->device);

        vkDestroyImageView(renderer->device, ws->color_image.view, 0);
        vmaDestroyImage(renderer->allocator, ws->color_image.image, ws->color_image.allocation);

        for (int j = 0; j < array_len(ws->image_views); ++j) {
            vkDestroyImageView(renderer->device, ws->image_views[j], 0);
        }

        vkDestroySwapchainKHR(renderer->device, ws->swapchain, 0);
        vkDestroySurfaceKHR(renderer->instance, ws->surface, 0);
        
        VkSurfaceKHR surface = w[i].create_surface(&w[i], renderer->instance);
        VkSwapchainKHR                  swapchain;
        VkFormat                        surface_format;
        dynarray VkImage                *images;
        dynarray VkImageView            *image_views;
        dynarray VD_RendererFrameData   *frame_data = ws->frame_data;
        VD_R_AllocatedImage             color_image;
        VD_R_AllocatedImage             depth_image;
        create_swapchain_image_views_and_framebuffers(
            ecs_get_name(it->world, it->entities[i]),
            it->entities[i],
            renderer,
            surface,
            (VkExtent2D) { sizes[i].x, sizes[i].y },
            &swapchain,
            &surface_format,
            &images,
            &image_views,
            &frame_data,
            &color_image,
            &depth_image);

        ecs_set(it->world, it->entities[i], WindowSurfaceComponent, {
            .swapchain = swapchain,
            .surface = surface,
            .surface_format = surface_format,
            .images = images,
            .image_views = image_views,
            .extent = { sizes[i].x, sizes[i].y },
            .frame_data = frame_data,
            .current_frame = 0,
            .color_image = color_image,
            .depth_image = depth_image,
        });
    }
}

void RendererGatherStaticMeshComponentSystem(ecs_iter_t *it)
{
    const Application *app = ecs_singleton_get(it->world, Application);
    VD_Renderer *renderer = vd_instance_get_renderer(app->instance);
    StaticMeshComponent *static_mesh_components = ecs_field(it, StaticMeshComponent, 0);
    WorldTransformComponent *world_transforms = ecs_field(it, WorldTransformComponent, 1);

    for (int i = 0; i < it->count; ++i) {
        RenderObject ro = {
            .mesh = static_mesh_components[i].mesh,
            .material = static_mesh_components[i].material,
        };

        glm_mat4_copy(world_transforms[i].world, ro.transform);
        vd_renderer_push_render_object(renderer, &ro);
    }
}
