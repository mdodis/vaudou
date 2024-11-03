#ifndef VD_CFG_H
#define VD_CFG_H
#include "vd_common.h"

const u32 VD_MM_Page_Size = 4096;
const u32 VD_MM_Max_Memory_Regions = 16;

#ifdef VD_ENABLE_VALIDATION_LAYERS
#define VD_VALIDATION_LAYERS 1
#endif

#ifndef VD_VALIDATION_LAYERS 
#define VD_VALIDATION_LAYERS 0
#endif

#ifdef VD_ENABLE_VULKAN_OBJECT_NAMES
#define VD_VULKAN_OBJECT_NAMES 1
#endif

#ifndef VD_VULKAN_OBJECT_NAMES
#define VD_VULKAN_OBJECT_NAMES 0
#endif

#ifdef VD_OPTION_ENABLE_VMA_TRACKING
#define VD_VMA_TRACKING 1
#endif

#ifndef VD_VMA_TRACKING
#define VD_VMA_TRACKING 0
#endif

#endif
