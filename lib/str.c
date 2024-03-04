#define VD_INTERNAL_SOURCE_FILE 1
#include "str.h"

#include <string.h>

VD_str vd_str_from_cstr(const char *in_cstr) 
{
    str result = {};
    size_t ln = strlen(in_cstr);

    if _impossible(ln > VD_SIZET_MAX) {
        result.len = VD_U32_MAX;
    } else {
        result.len = (u32)ln;
    }

    result.data = (char *)in_cstr;
    return result;
}

bool vd_str_eq(VD_str a, VD_str b)
{
    if (a.len != b.len) {
        return false;
    }

    return memcmp(a.data, b.data, a.len) == 0;
}

u32 vd_str_last_of(VD_str s, char c)
{
    u32 i = s.len;
    while (i) {
        --i;
        if (s.data[i] == c) {
            return i;
        }
    }

    return s.len;
}

u32 vd_str_last_of_s(VD_str s, VD_str c, u32 start)
{
    if (s.len == 0) return s.len;

    start = s.len < start ? s.len : start;

    u32 sindex = c.len;
    u32 i      = start;
    while (i != 0) {
        i--;
        sindex--;

        if (s.data[i] != c.data[sindex]) {
            sindex = c.len;
        }

        if (sindex == 0) {
            return i;
        }
    }

    return s.len;
}

VD_bool vd_str_split(VD_str s, u32 at, VD_str *left, VD_str *right)
{
    if ((s.len < at) || (at >= s.len)) {
        return false;
    }

    left->data = s.data;
    left->len  = at + 1;
    right->data = s.data + at + 1;
    right->len  = s.len - (at + 1);

    return true;
}

VD_bool vd_str_ends_with(VD_str s, VD_str needle)
{
    return vd_str_last_of_s(s, needle, s.len) == (s.len - needle.len);
}

