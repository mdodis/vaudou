#define VD_INTERNAL_SOURCE_FILE 1
#include "cvar.h"
#include "strmap.h"

#include <stdlib.h>
#include <assert.h>

struct VD_CVS {
    strmap VD_CVarValue             *map;
    VD_HOOK(VD_CVarChangedDelegate) on_cvar_update;
};

VD_CVS *vd_cvs_create()
{
    return calloc(1, sizeof(VD_CVS));
}

void vd_cvs_init(VD_CVS *cvs)
{
    strmap_init(cvs->map, vd_memory_get_system_allocator());
    VD_HOOK_INIT(cvs->on_cvar_update, vd_memory_get_system_allocator());
}

void vd_cvs_register_hook(VD_CVS *cvs, VD_CALLBACK(VD_CVarChangedDelegate) *callback)
{
    VD_HOOK_SUBSCRIBE(cvs->on_cvar_update, callback->entry.func, callback->entry.usrdata);
}

void vd_cvs_unregister_hook(VD_CVS *cvs, VD_CALLBACK(VD_CVarChangedDelegate) *callback)
{
    VD_HOOK_UNSUBSCRIBE(cvs->on_cvar_update, callback->entry.func);
}

VD_bool vd_cvs_get(VD_CVS *cvs, VD_str name, VD_CVarValue *cvar)
{
    return strmap_get(cvs->map, name, cvar);
}

void vd_cvs_set(VD_CVS *cvs, VD_str name, VD_CVarValue value)
{
    VD_CVarValue cvar;
    if (strmap_get(cvs->map, name, &cvar)) {
        assert(cvar.type == value.type);
        memcpy(&cvar, &value, sizeof(value));
        VD_HOOK_INVOKE(cvs->on_cvar_update, name, &cvar);
    } else {
        memcpy(&cvar, &value, sizeof(value));
        strmap_set(cvs->map, name, &cvar);
    }
}

void vd_cvs_deinit(VD_CVS *cvs)
{
    strmap_deinit(cvs->map);
    VD_HOOK_DEINIT(cvs->on_cvar_update);
}
