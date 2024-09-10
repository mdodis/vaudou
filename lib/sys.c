#define VD_SYSUTIL_IMPLEMENTATION
#define VD_INTERNAL_SOURCE_FILE 1
#include "sys.h"
#include "vd_sysutil.h"

VD_str vd_get_exec_path(VD_Arena *arena)
{
    VD_str result = {0};
    vd_sysutil_get_executable_path(0, &result.len);
    result.data = arena_alloc(arena, result.len);
    vd_sysutil_get_executable_path((char*)result.data, &result.len);
	for (int i = 0; i < result.len; i++)
	{
        if (result.data[i] == '\\') {
            result.data[i] = '/';
        }
	}

    result.len -= 1;
    return result;
}
