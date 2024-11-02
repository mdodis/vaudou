#ifndef VD_HANDLEMAP_H
#define VD_HANDLEMAP_H

#include "vd_common.h"

#define VD_HANDLEMAP

typedef struct {
    size_t          initial_capacity;
    VD_Allocator    *allocator;
    void            (*on_free_object)(void *object, void *c);
    void            *c;
} VD_HandleMapInitInfo;

typedef struct VD_HandleMap VD_HandleMap;

typedef struct {
    u64             id;
    VD_HandleMap    *map;
} VD_Handle;

typedef enum {
    /** The object will never be freed. */
    VD_HANDLEMAP_REF_MODE_ALWAYS  = 0,

    /** The object will be freed after its reference count is zero. */
    VD_HANDLEMAP_REF_MODE_COUNT   = 1,
} VD_HandleMapRefMode;

typedef enum {
    VD_HANDLEMAP_USE_MODE_DEFAULT = 0,
    VD_HANDLEMAP_USE_MODE_FRAME   = 1,
} VD_HandleMapUseMode;

typedef struct {
    VD_HandleMapRefMode ref_mode;
} VD_HandleMapRegisterInfo;

/**
 * @brief Initialize a handlemap
 * @param m The handle map pointer
 * @param info The VD_HandleMapInitInfo pointer
 * @return The newly created handlemap pointer
 */
#define VD_HANDLEMAP_INIT(m, ...) \
    (m = vd_handlemap__init(sizeof(*m), &(VD_HandleMapInitInfo)__VA_ARGS__))

#define VD_HANDLEMAP_DEINIT(m) \
    (vd_handlemap__deinit((VD_HandleMap*)(m)))

/**
 * @brief Create a handle with an object
 */
#define VD_HANDLEMAP_REGISTER(m, valueptr, ...) \
    (vd_handlemap__register((VD_HandleMap*)m,   \
    (void*)valueptr,                            \
    &(VD_HandleMapRegisterInfo)__VA_ARGS__))

void *vd_handlemap__init(size_t elsize, VD_HandleMapInitInfo *info);

VD_Handle vd_handlemap__register(VD_HandleMap *hdr, void *valueptr, VD_HandleMapRegisterInfo *info);

void *vd_handle_use(VD_Handle *handle, VD_HandleMapUseMode mode);

VD_Handle vd_handlemap_copy(VD_Handle *handle);

void vd_handle_drop(VD_Handle *handle);

void vd_handlemap__deinit(VD_HandleMap *map);

#define HandleOf(t) VD_Handle
#define USE_HANDLE(h, t) ((t*)vd_handle_use(&h, 0))
#define COPY_HANDLE(h)   vd_handlemap_copy(&(h))
#define DROP_HANDLE(h)   (vd_handle_drop(&(h)))
#define INVALID_HANDLE() ((VD_Handle) {0})

#ifdef VD_ABBREVIATIONS
#define handlemap VD_HANDLEMAP
#define Handle    VD_Handle
#endif

#endif // !VD_HANDLEMAP_H
