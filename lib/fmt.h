#ifndef VD_FMT_INTEGRATION_H
#define VD_FMT_INTEGRATION_H
#include "common.h"
#include "vd_fmt.h"
#include "arena.h"
#include "str.h"

VD_INLINE VD_str vd_snfmt(VD_Arena *a, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    size_t ret = vd_fmt_vsnfmt(0, 0, fmt, args);
    va_end(args);

    char *data = (char*)VD_ARENA_ALLOC(a, ret);
    va_start(args, fmt);
    ret = vd_fmt_vsnfmt(data, ret, fmt, args);
    va_end(args);

    return VD_str { data, (u32)ret };
}

#endif