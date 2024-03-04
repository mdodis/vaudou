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

VD_bool vd_str_eq(VD_str a, VD_str b);
u32 vd_str_last_of(VD_str s, char c);
u32 vd_str_last_of_s(VD_str s, VD_str c, u32 start);
VD_bool vd_str_split(VD_str s, u32 at, VD_str *left, VD_str *right);
VD_bool vd_str_ends_with(VD_str s, VD_str suffix);
VD_INLINE VD_str vd_str_chop_right(VD_str s, u32 at)
{
    VD_str left, right;
    if (vd_str_split(s, at, &left, &right)) {
        return left;
    }

    return s;
}

VD_INLINE VD_str vd_str_chop_right_last_of(VD_str s, char c)
{
    return vd_str_chop_right(s, vd_str_last_of(s, c) - 1);
}

#if VD_ABBREVIATIONS
#define str                     VD_str
#define str_from_cstr           vd_str_from_cstr
#define str_lit                 VD_STR_LIT
#define str_eq                  vd_str_eq
#define str_last_of             vd_str_last_of
#define str_split               vd_str_split
#define str_chop_right          vd_str_chop_right
#define str_chop_right_last_of  vd_str_chop_right_last_of
#define str_ends_with           vd_str_ends_with
#endif

#endif