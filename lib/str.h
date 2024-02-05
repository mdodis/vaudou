#ifndef VD_STR_H
#define VD_STR_H

#include "common.h"

/**
 * @brief An immutable string slice.
 */
typedef struct {
    cstr data;
    u32 len;
} VD_str;

/**
 * @brief Create a string slice from a null-terminated string.
 * 
 * @param str The null-terminated string.
 * @return VD_str The string slice.
 */
VD_str vd_str_from_cstr(const char *in_cstr);

#if VD_ABBREVIATIONS
#define str VD_str
#define str_from_cstr vd_str_from_cstr
#endif

#endif