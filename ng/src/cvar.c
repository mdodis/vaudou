#define VD_INTERNAL_SOURCE_FILE 1
#include "cvar.h"
#include "strmap.h"

static struct {
    strmap VD_CVarValue *map;
} sys;

bool sys_cvar_init() 
{
    strmap_init(sys.map, vd_memory_get_system_allocator());
    return true;
}
