#define VD_FMT_IMPLEMENTATION
#include "fmt.h"

VD_str vd_snfmt(VD_Arena *a, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    size_t ret = vd_fmt_vsnfmt(0, 0, fmt, args);
    va_end(args);

    char *data = (char*)VD_ARENA_ALLOC(a, ret);
    va_start(args, fmt);
    ret = vd_fmt_vsnfmt(data, ret, fmt, args);
    va_end(args);

    return (VD_str) { data, (u32)ret };
}

VD_str vd_snfmt_a(VD_Allocator *a, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t ret = vd_fmt_vsnfmt(0, 0, fmt, args);
    va_end(args);

    char *data = (char *)vd_realloc(a, 0, 0, ret);
    va_start(args, fmt);
    ret = vd_fmt_vsnfmt(data, ret, fmt, args);
    va_end(args);

    return (VD_str) { data, (u32)ret };
}