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

#define VD_STR_LIT(s) ((VD_str){s, sizeof(s) - 1})

/**
 * @brief Create a string slice from a null-terminated string.
 * 
 * @param str The null-terminated string.
 * @return VD_str The string slice.
 */
VD_str vd_str_from_cstr(const char *in_cstr);

bool vd_str_eq(VD_str a, VD_str b);

#if VD_ABBREVIATIONS
#define str             VD_str
#define str_from_cstr   vd_str_from_cstr
#define str_lit         VD_STR_LIT
#endif

#endif