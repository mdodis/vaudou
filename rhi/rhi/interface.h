#ifndef VD_RHI_H
#define VD_RHI_H
#include "vd_common.h"

typedef uintptr_t VD(RHHandle);

typedef VD(RHHandle) VD(RHPhysicalDevice);
typedef VD(RHHandle) VD(RHDevice);
typedef VD(RHHandle) VD(RHSurface);
typedef VD(RHHandle) VD(RHPresentationSurface);
typedef VD(RHHandle) VD(RHBuffer);
typedef VD(RHHandle) VD(RHAllocation);

typedef struct WindowComponent WindowComponent;

typedef struct {
    const char *file;
    int         line;
} VD(RHAllocationTracking);

#define VD_RHI_ALLOCATION_TRACKING() & (VD(RHAllocationTracking)) \
    { \
        .file = __FILE__, \
        .line = __LINE__, \
    }

typedef struct VD(RHI) VD(RHI);

typedef enum {
    VD_(RH_RESULT_SUCCESS) = 0,
} VD(RHResult);

typedef enum {
    VD_(RH_MEMORY_USAGE_UNKNOWN)    = 0,
    VD_(RH_MEMORY_USAGE_GPU_ONLY)   = 1,
    VD_(RH_MEMORY_USAGE_CPU_ONLY)   = 2,
    VD_(RH_MEMORY_USAGE_CPU_TO_GPU) = 3,
    VD_(RH_MEMORY_USAGE_GPU_TO_CPU) = 4,
} VD(RHMemoryUsage);

typedef enum {
    VD_(RH_BUFFER_USAGE_TRANSFER_SRC_BIT)           = 0x00000001,
    VD_(RH_BUFFER_USAGE_TRANSFER_DST_BIT)           = 0x00000002,
    VD_(RH_BUFFER_USAGE_UNIFORM_TEXEL_BIT)          = 0x00000004,
    VD_(RH_BUFFER_USAGE_STORAGE_TEXEL_BIT)          = 0x00000008,
    VD_(RH_BUFFER_USAGE_UNIFORM_BUFFER_BIT)         = 0x00000010,
    VD_(RH_BUFFER_USAGE_STORAGE_BUFFER_BIT)         = 0x00000020,
    VD_(RH_BUFFER_USAGE_INDEX_BUFFER_BIT)           = 0x00000040,
    VD_(RH_BUFFER_USAGE_VERTEX_BUFFER_BIT)          = 0x00000080,
    VD_(RH_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)  = 0x00020000,
} VD(RHBufferUsage);

#define VD_RHI_IS_PHYSICAL_DEVICE_SUITABLE(name) \
    VD(RHResult) name( \
        VD(RHI) *rhi, \
        VD(RHPhysicalDevice) physical_device, \
        int *out_is_suitable, \
        void *c)

typedef VD_RHI_IS_PHYSICAL_DEVICE_SUITABLE(VD(RHIsPhysicalDeviceSuitableProc));

#define VD_RHI_CREATE_SURFACE(name) \
    VD(RHResult) name( \
        VD(RHI) *rhi, \
        WindowComponent *window_component, \
        VD(RHSurface) *out_surface, \
        void *c)
typedef VD_RHI_CREATE_SURFACE(VD(RHCreateSurfaceProc));

/**
 * init
 */
typedef struct {
    VD(RHIsPhysicalDeviceSuitableProc) *is_physical_device_suitable;
    VD(RHCreateSurfaceProc)            *create_surface;
    void                               *c;

    VD_Allocator                       *frame_allocator;

    struct {
        int                            debug;

        union {
            struct {
                u32                    num_instance_extensions;
                const char             **instance_extensions;
            } vulkan;
        } api_specific;
    } extensions;
} VD(RHInitInfo);
#define VD_RHI_INIT_PROC(name) VD(RHResult) name(VD(RHI) *rhi, VD(RHInitInfo) *info)
typedef VD_RHI_INIT_PROC(VD(RHInitProc));

/**
 *  deinit
 */
typedef struct {
    /** Try to deinit quickly, without taking care to correctly free resources. */
    int be_quick;
} VD(RHDeinitInfo);
#define VD_RHI_DEINIT_PROC(name) void name(VD(RHI) *rhi, VD(RHDeinitInfo) *info)
typedef VD_RHI_DEINIT_PROC(VD(RHDeinitProc));

/**
 * create_presentation_surface
 */
typedef struct {
    WindowComponent *window_component;
} VD(RHCreatePresentationSurfaceInfo);
#define VD_RHI_CREATE_PRESENTATION_SURFACE_PROC(name) \
    VD(RHResult) name( \
        VD(RHI) *rhi, \
        VD(RHCreatePresentationSurfaceInfo) *info, \
        VD(RHPresentationSurface) *out_surface)

/**
 * create_buffer
 */
typedef struct {
    size_t              size;
    VD(RHMemoryUsage)   memory_usage;
    VD(RHBufferUsage)   buffer_usage;
} VD(RHCreateBufferInfo);
#define VD_RHI_CREATE_BUFFER_PROC(name) \
    VD(RHResult) name(\
        VD(RHI) *rhi, \
        VD(RHCreateBufferInfo) *info, \
        VD(RHBuffer) *out_buffer, \
        VD(RHAllocation) *out_allocation)
typedef VD_RHI_CREATE_BUFFER_PROC(VD(RHCreateBufferProc));

/**
 * destroy_buffer
 */
typedef struct {
    VD(RHBuffer)        *buffer;
    VD(RHAllocation)    *allocation;
} VD(RHDestroyBufferInfo);
#define VD_RHI_DESTROY_BUFFER_PROC(name) void name(VD(RHI) *rhi, VD(RHDestroyBufferInfo) *info)
typedef VD_RHI_DESTROY_BUFFER_PROC(VD(RHDestroyBufferProc));

/**
 * map
 */
#define VD_RHI_MAP_PROC(name) void *name(VD(RHI) *rhi, VD(RHAllocation) *allocation);
typedef VD_RHI_MAP_PROC(VD(RHMapProc));

/**
 * unmap
 */
#define VD_RHI_UNMAP_PROC(name) void name(VD(RHI) *rhi, VD(RHAllocation) *allocation);
typedef VD_RHI_UNMAP_PROC(VD(RHUnmapProc));

/**
 * get_size
 */
#define VD_RHI_GET_SIZE_PROC(name) size_t name(VD(RHI) *rhi, VD(RHAllocation) *allocation);
typedef VD_RHI_GET_SIZE_PROC(VD(RHGetSizeProc));

struct VD(RHI) {
    VD(RHInitProc)          *initialize;
    VD(RHDeinitProc)        *deinitialize;
    VD(RHCreateBufferProc)  *create_buffer;
    VD(RHDestroyBufferProc) *destroy_buffer;
    VD(RHMapProc)           *map;
    VD(RHUnmapProc)         *unmap;
    VD(RHGetSizeProc)       *get_size;
    VD(RHCreateSurfaceProc) *create_presentation_surface;
    void                    *c;
};

#endif // !VD_RHI_H
