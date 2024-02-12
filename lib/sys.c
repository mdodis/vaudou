#define VD_INTERNAL_SOURCE_FILE 1
#include "sys.h"

#if VD_PLATFORM_MACOS
#include <mach-o/dyld.h>
#endif

VD_str vd_get_exec_path(VD_Arena *arena)
{
    VD_str result = {0};
    _NSGetExecutablePath(0, &result.len);
    result.data = arena_alloc(arena, result.len);
    _NSGetExecutablePath(result.data, &result.len);
    return result;
}
