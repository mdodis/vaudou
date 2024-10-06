#ifndef VD_STR_H
#define VD_STR_H

#include "vd_common.h"

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

VD_INLINE VD_str vd_str_chop_left(VD_str s, u32 at)
{
    VD_str left, right;
    if (vd_str_split(s, at, &left, &right)) {
        return right;
    }

    return s;
}

VD_INLINE u32 vd_str_last_of_any(VD_str s, char array[], u32 len)
{
    u32 ret = s.len;
    for (u32 i = 0; i < len; ++i) {
        u32 result = vd_str_last_of(s, array[i]);
        if (result == s.len) {
            continue;
        }

        if ((result > ret) || (ret == s.len)) {
            ret = result;
        }
    }

    return ret;
}

VD_INLINE u32 vd_str_last_of_any_s(VD_str s, VD_str array[], u32 len)
{
    u32 ret = s.len;
    for (u32 i = 0; i < len; ++i) {
        u32 result = vd_str_last_of_s(s, array[i], s.len);
        if (result == s.len) {
            continue;
        }

        if ((result > ret) || (ret == s.len)) {
            ret = result;
        }
    }

    return ret;
}

VD_INLINE VD_str vd_str_path_last_part(VD_str s) {
    u32 last_sep = vd_str_last_of_any(s, (char[]){'/', '\\'}, 2);
    return vd_str_chop_left(s, last_sep);
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
#define str_chop_left           vd_str_chop_left
#define str_last_of_any         vd_str_last_of_any
#define str_last_of_any_s       vd_str_last_of_any_s
#define str_ends_with           vd_str_ends_with
#define str_path_last_part      vd_str_path_last_part
#endif

#endif