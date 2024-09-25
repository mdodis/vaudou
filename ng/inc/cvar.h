#ifndef VD_CVAR_H
#define VD_CVAR_H

#include "common.h"
#include "delegate.h"
#include "str.h"

typedef struct VD_CVS VD_CVS;

typedef enum {
    VD_CVS_I32,
    VD_CVS_F32,
    VD_CVS_BOOL,
} VD_CVarType;

typedef struct {
    VD_CVarType type;
    union {
        i32  i;
        f32  f;
        bool b;
    } v;
} VD_CVarValue;


VD_DELEGATE_DECLARE_PARAMS2_VOID(VD_CVarChangedDelegate, VD_str, name, VD_CVarValue *, var)

VD_CVS *vd_cvs_create();
void vd_cvs_init(VD_CVS *cvs);
void vd_cvs_register_hook(VD_CVS *cvs, VD_CALLBACK(VD_CVarChangedDelegate) *callback);
void vd_cvs_unregister_hook(VD_CVS *cvs, VD_CALLBACK(VD_CVarChangedDelegate) *callback);
VD_bool vd_cvs_get(VD_CVS *cvs, VD_str name, VD_CVarValue *cvar);
void vd_cvs_set(VD_CVS *cvs, VD_str name, VD_CVarValue value);
void vd_cvs_deinit(VD_CVS *cvs);

/**
 * @brief Get a cvar
 * @param name  The name of the cvar
 * @param cvar  A pointer to @see VD_CVarValue
 * @return      True if the cvar was found, false otherwise
 */
#define VD_CVS_GET(name, cvar) \
    vd_cvs_get(vd_instance_get_cvs(vd_instance_get()), VD_STR_LIT(name), cvar)

#define VD_CVS_GET_INT(name, vptr)                                          \
    do                                                                      \
    {                                                                       \
        VD_CVarValue __cvar;                                                \
        VD_CVS_GET(name, &__cvar);                                          \
        assert(__cvar.type == VD_CVS_I32);                                  \
        *vptr = __cvar.v.i;                                                 \
    } while(0)

/**
 * @brief Set a cvar
 * @param name  The name of the cvar
 * @param value The value of the cvar
 */
#define VD_CVS_SET(name, value) \
    vd_cvs_set(vd_instance_get_cvs(vd_instance_get()), VD_STR_LIT(name), value)

#define VD_CVS_SET_INT(name, value) \
    VD_CVS_SET(name, ((VD_CVarValue) {.type = VD_CVS_I32, .v.i = value}))

#define VD_CVS_SET_FLOAT(name, value) \
    VD_CVS_SET(name, (VD_CVarValue) {.type = VD_CVS_F32, .v.f = value})

#define VD_CVS_SET_BOOL(name, value) \
    VD_CVS_SET(name, (VD_CVarValue) {.type = VD_CVS_BOOL, .v.b = value})

/**
 * @brief Register a hook for cvar changes
 * @param infptr    The function pointer
 * @param inusrdata The user data
 */
#define VD_CVS_REGISTER_HOOK(infptr, inusrdata) \
    vd_cvs_register_hook(\
        vd_instance_get_cvs(vd_instance_get()), \
        &(VD_CALLBACK(VD_CVarChangedDelegate)) {infptr, inusrdata})

/**
 * @brief Unregister a hook for cvar changes
 * @param infptr    The function pointer
 */
#define VD_CVS_UNREGISTER_HOOK(infptr) \
    vd_cvs_unregister_hook(\
        vd_instance_get_cvs(vd_instance_get()), \
        &(VD_CALLBACK(VD_CVarChangedDelegate)) {infptr, 0})

#endif