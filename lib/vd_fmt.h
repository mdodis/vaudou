// vd_fmt.h - A simple string formatting library
//
// STRING FORMATTING
// - vd_fmt_vsnfmt(out, n, "The number is %{i32}", 12)
//     - Prints "The number is 12"
// - vd_fmt_vsnfmt(out, n, "The number is %{what}")
//     - Prints "The number is "
//     - Throws error
#ifndef VD_FMT_H
#define VD_FMT_H
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define VD_FMT_IMPLEMENTATION

#define VD_FMT_PROC_FMT(name) size_t name(char *out, size_t n, va_list a)
typedef VD_FMT_PROC_FMT(VD_FmtProcFmt);

typedef struct _tagVD_FmtStr {
    char         *dat;
    unsigned int  len;
} VD_FmtStr;

typedef struct _tagVD_FmtTable {
    VD_FmtStr       s;
    VD_FmtProcFmt  *p;
} VD_FmtTable;

void vd_fmt_set_lut(VD_FmtTable *lut, size_t n);
size_t vd_fmt_vsnfmt(char *out, size_t n, const char *fmt, va_list args);

static inline size_t vd_fmt_snfmt(char *out, size_t n, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t ret = vd_fmt_vsnfmt(out, n, fmt, args);
    va_end(args);
    return ret;
}

static inline void vd_fmt_printf(const char *fmt, ...)
{
    static char buf[4096];

    va_list args;
    va_start(args, fmt);
    size_t ret = vd_fmt_vsnfmt(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (ret > 0) {
        fwrite(buf, 1, ret, stdout);
    }
}

#endif

#ifdef VD_FMT_IMPLEMENTATION

#define _VD_FMT_LIT(s) (VD_FmtStr) {s, sizeof(s) - 1}

static VD_FMT_PROC_FMT(_vd_fmt_def_i32);

static VD_FmtTable _VD_Fmt_Def_Lut[] = {
    (VD_FmtTable) { _VD_FMT_LIT("i32"),     _vd_fmt_def_i32 },
    (VD_FmtTable) { _VD_FMT_LIT("i64"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("u32"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("u64"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("f32"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("f64"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("cstr"),    NULL },
    (VD_FmtTable) { _VD_FMT_LIT("str"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("ptr"),     NULL },
    (VD_FmtTable) { _VD_FMT_LIT("ptrdiff"), NULL },
    (VD_FmtTable) { _VD_FMT_LIT("size"),    NULL },
    (VD_FmtTable) { _VD_FMT_LIT("usize"),   NULL },
    (VD_FmtTable) { _VD_FMT_LIT("bool"),    NULL },
    (VD_FmtTable) { _VD_FMT_LIT("char"),    NULL },
    (VD_FmtTable) { _VD_FMT_LIT("void"),    NULL },
};

static struct {
    VD_FmtTable *lut;
    size_t       n;
} _vd_g = {
    _VD_Fmt_Def_Lut,
    sizeof(_VD_Fmt_Def_Lut) / sizeof(_VD_Fmt_Def_Lut[0]),
};

void vd_fmt_set_lut(VD_FmtTable *lut, size_t n)
{
    _vd_g.lut = lut;
    _vd_g.n = n;
}

size_t vd_fmt_vsnfmt(char *out, size_t n, const char *fmt, va_list args)
{
    const char *cf = fmt;
    size_t nwrite = 0;

    cf = fmt;
    while (*cf) {
        if (*cf != '%') {
            if (nwrite < n) {
                out[nwrite] = *cf;
            }

            cf++;
            nwrite++;
            continue;
        }

        if (*(cf + 1) == '{') {
            const char *beg = cf + 2;
            const char *end = cf + 2;
            while (*end && *end != '}') {
                end++;
            }

            if (*end != '}') {
                return 0;
            }

            ptrdiff_t cl = end - beg;
            size_t i = 0;
            int found = 0;
            while (i < _vd_g.n) {
                if ((_vd_g.lut[i].s.len == cl) &&
                    (strncmp(_vd_g.lut[i].s.dat, beg, cl) == 0)) 
                {
                    size_t available = n;
                    if (n < nwrite) {
                        available = 0;
                    } else {
                        available = n - nwrite;
                    }

                    nwrite += _vd_g.lut[i].p(out + nwrite, available, args);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                return 0;
            }

            cf = end + 1;
        } else {
            if (nwrite < n) {
                out[nwrite] = *cf;
            }
            nwrite++;
            cf++;
        }
    }

    return nwrite;
}

static VD_FMT_PROC_FMT(_vd_fmt_def_i32)
{
    int32_t i = va_arg(a, int32_t);
    return snprintf(out, n, "%d", i);
}

#endif