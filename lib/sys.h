#ifndef VD_SYSTEM_H
#define VD_SYSTEM_H
#include "str.h"
#include "allocator.h"
#include "arena.h"
#include "vd_sysutil.h"

VD_str vd_get_exec_path(VD_Arena *arena);

#endif