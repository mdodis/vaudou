#ifndef VD_FMT_INTEGRATION_H
#define VD_FMT_INTEGRATION_H
#include "common.h"
#include "vd_fmt.h"
#include "arena.h"
#include "str.h"

VD_str vd_snfmt(VD_Arena *a, const char *fmt, ...);

#endif