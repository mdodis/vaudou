#ifndef VD_CVAR_H
#define VD_CVAR_H

#include "common.h"

typedef enum {
    Int32,
    Bool,
    Float,
} VD_CVarType;

typedef struct {
    VD_CVarType type;
    union {
        i32  i;
        f32  f;
        bool b;
    } v;
} VD_CVarValue;

VD_bool sys_cvar_init();

#endif