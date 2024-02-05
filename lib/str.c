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
