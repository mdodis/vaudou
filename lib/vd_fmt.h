// vd_fmt.h - A string formatting library
//
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// STRING FORMATTING
// - vd_fmt_vsnfmt(out, n, "The number is %{i32}", 12)
//     - Prints "The number is 12"
// - vd_fmt_vsnfmt(out, n, "The number is %{what}")
//     - Throws error
//
// BUILTINS
// - %{i8}             int8_t
// - %{i16}            int16_t
// - %{i32}            int32_t
// - %{i64}            int64_t
// - %{u8}             uint8_t
// - %{u16}            uint16_t
// - %{u32}            uint32_t
// - %{u64}            uint64_t
// - %{f32}            float
// - %{f64}            double
// - %{str(i,u)<j>}    A string slice of the type struct { char *data; (i,u)<j> len; };
// - %{pathu32}        Like %{str(i,u)<j>} but prints a unix path (replaces '\\' with '/').
// - %{null}           Prints null terminator ('\0')
//
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_FMT_IMPLEMENTATION
#endif

#ifndef VD_FMT_H
#define VD_FMT_H
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define VD_FMT_PROC_FMT(name) size_t name(char *out, size_t n, va_list *a)
typedef VD_FMT_PROC_FMT(VD_FmtProcFmt);

typedef struct _tagVD_FmtStr {
    char         *dat;
	unsigned int  len;         // Length of the string + 1
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

static inline void vd_fmt_vfprintf(FILE* f, const char *fmt, va_list args)
{
    char buf[4096];
    size_t ret = vd_fmt_vsnfmt(buf, sizeof(buf), fmt, args);
    if (ret > 0) {
        fwrite(buf, 1, ret, f);
    }
}

static inline void vd_fmt_fprintf(FILE* f, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vd_fmt_vfprintf(f, fmt, args);
    va_end(args);
}

static inline void vd_fmt_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vd_fmt_vfprintf(stdout, fmt, args);
    va_end(args);
}

#ifdef VD_FMT_IMPLEMENTATION

#define VD_FMT_LIT(s) {s, (unsigned int)sizeof(s) - 1}

/* ----INTEGERS---------------------------------------------------------------------------------- */
#define VD_FMT_NUM_DIGITS(x) _Generic((x),         \
    uint64_t: sizeof("18446744073709551616") - 1,  \
    uint32_t: sizeof("4294967295") - 1,            \
    uint16_t: sizeof("65535") - 1,                 \
    uint8_t:  sizeof("255") - 1,                   \
    int64_t:  sizeof("+9223372036854775808") - 1,  \
    int32_t:  sizeof("+2147483648"), - 1,          \
    int16_t:  sizeof("+32768") - 1,                \
    int8_t:   sizeof("+128") - 1,                  \
    default:  0                                    \
    )

#define VD_FMT__INT_CONVERSION_PROCS      \
    VD_FMT__X(uint64_t, 0, 20, uint64_t)  \
    VD_FMT__X(int64_t,  1, 21, int64_t)   \
    VD_FMT__X(uint32_t, 0, 10, uint32_t)  \
    VD_FMT__X(int32_t,  1, 11, int32_t)   \
    VD_FMT__X(uint16_t, 0, 5,  uint32_t)  \
    VD_FMT__X(int16_t,  1, 6,  int32_t)   \
    VD_FMT__X(uint8_t,  0, 3,  uint32_t)  \
    VD_FMT__X(int8_t,   1, 4,  int32_t)

#define VD_FMT__INT_CONVERSION_PROC_NAME2(x, y, z) x##y##z
#define VD_FMT__INT_CONVERSION_PROC_NAME(x) VD_FMT__INT_CONVERSION_PROC_NAME2(vd_fmt__,x,_to_str)
#define VD_FMT__INT_CONVERSION_PROC_SIG(intname) \
    VD_FMT_PROC_FMT(VD_FMT__INT_CONVERSION_PROC_NAME(intname))

static const char VD_FMT__Digits_Lut[201] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

#define VD_FMT__X(intname, issigned, maxdigits, argcast)                      \
    static VD_FMT__INT_CONVERSION_PROC_SIG(intname)                           \
    {                                                                         \
        char wr[21];                                                          \
        const size_t length = maxdigits;                                      \
        size_t next = length - 1;                                             \
        if (n < next) return length;                                          \
        intname v = (intname)va_arg(*a, argcast);                             \
        int sgn = 0;                                                          \
        if (issigned && (v < 0)) {                                            \
            sgn = 1;                                                          \
            v = -v;                                                           \
        }                                                                     \
        while (v >= 100) {                                                    \
            intname i = (v % 100) * 2;                                        \
            v /= 100;                                                         \
            wr[next] = VD_FMT__Digits_Lut[i + 1];                             \
            wr[next - 1] = VD_FMT__Digits_Lut[i];                             \
            next -= 2;                                                        \
        }                                                                     \
        if (v < 10) {                                                         \
            wr[next] = '0' + v;                                               \
        } else {                                                              \
            intname i = v * 2;                                                \
            wr[next--] = VD_FMT__Digits_Lut[i + 1];                           \
            wr[next] = VD_FMT__Digits_Lut[i];                                 \
        }                                                                     \
        if (sgn) {                                                            \
            wr[--next] = '-';                                                 \
        }                                                                     \
        if (n < (length - next)) return length - next;                        \
        memcpy(out, wr + next, length - next);                                \
        return length - next;                                                 \
    }                                                                         \

VD_FMT__INT_CONVERSION_PROCS

#undef VD_FMT__X

/* ----STRING SLICES----------------------------------------------------------------------------- */
#define VD_FMT__STRING_SLICE_PROC_NAME(lentype) VD_FMT__string_slice_to_str##lentype
#define VD_FMT__STRING_SLICE_PROC_SIG(lentype) \
    VD_FMT_PROC_FMT(VD_FMT__STRING_SLICE_PROC_NAME(lentype))

#define VD_FMT__STRING_SLICE_PROCS \
    VD_FMT__X(u32, uint32_t)       \
    VD_FMT__X(u64, uint64_t)       \

#define VD_FMT__X(lentype, argcast)                                        \
    static VD_FMT__STRING_SLICE_PROC_SIG(lentype)                          \
    {                                                                      \
        struct _vd_fmt_str_slice { char *data; argcast len; };             \
        struct _vd_fmt_str_slice v = va_arg(*a, struct _vd_fmt_str_slice); \
        if (n < v.len) return v.len;                                       \
        memcpy(out, v.data, v.len);                                        \
        return v.len;                                                      \
    }                                                                      \

VD_FMT__STRING_SLICE_PROCS

#undef VD_FMT__X

/* ----FLOATING POINT---------------------------------------------------------------------------- */
static VD_FMT_PROC_FMT(vd_fmt__f32_to_str)
{
    float f = va_arg(*a, float);
    return snprintf(out, n, "%f", f);
}

static VD_FMT_PROC_FMT(vd_fmt__f64_to_str)
{
    double f = va_arg(*a, double);
    return snprintf(out, n, "%f", f);
}

/* ----PATHS------------------------------------------------------------------------------------- */
static VD_FMT_PROC_FMT(vd_fmt__pathu32_to_str)
{
    struct _vd_fmt_str_slice { char* data; uint32_t len; };
	struct _vd_fmt_str_slice v = va_arg(*a, struct _vd_fmt_str_slice);
	if (n < v.len) return v.len;

    for (int i = 0; i < v.len; ++i) {
        if (v.data[i] == '\\') {
			out[i] = '/';
        } else {
			out[i] = v.data[i];
        }
    }
    return v.len;
}

static VD_FMT_PROC_FMT(vd_fmt__null_to_str)
{
    if (n < 1) return 1;

	out[0] = '\0';

    return 1;
}

static VD_FMT_PROC_FMT(vd_fmt__cstr)
{
    const char *s = va_arg(*a, const char*);
    size_t len = strlen(s);
    if (n < len) return len;
    memcpy(out, s, len);
    return len;
}

static const VD_FmtTable _VD_Fmt_Def_Lut[] = {
    { VD_FMT_LIT("i8"),      VD_FMT__INT_CONVERSION_PROC_NAME(int8_t)   },
    { VD_FMT_LIT("i16"),     VD_FMT__INT_CONVERSION_PROC_NAME(int16_t)  },
    { VD_FMT_LIT("i32"),     VD_FMT__INT_CONVERSION_PROC_NAME(int32_t)  },
    { VD_FMT_LIT("i64"),     VD_FMT__INT_CONVERSION_PROC_NAME(int64_t)  },
    { VD_FMT_LIT("u8"),      VD_FMT__INT_CONVERSION_PROC_NAME(uint8_t)  },
    { VD_FMT_LIT("u16"),     VD_FMT__INT_CONVERSION_PROC_NAME(uint16_t) },
    { VD_FMT_LIT("u32"),     VD_FMT__INT_CONVERSION_PROC_NAME(uint32_t) },
    { VD_FMT_LIT("u64"),     VD_FMT__INT_CONVERSION_PROC_NAME(uint64_t) },
    { VD_FMT_LIT("f32"),     vd_fmt__f32_to_str },
    { VD_FMT_LIT("f64"),     vd_fmt__f64_to_str },
    { VD_FMT_LIT("stru32"),  VD_FMT__STRING_SLICE_PROC_NAME(u32) },
    { VD_FMT_LIT("stru64"),  VD_FMT__STRING_SLICE_PROC_NAME(u64) },
    { VD_FMT_LIT("cstr"),    vd_fmt__cstr },
    { VD_FMT_LIT("ptr"),     NULL },
    { VD_FMT_LIT("ptrdiff"), NULL },
    { VD_FMT_LIT("size"),    NULL },
    { VD_FMT_LIT("usize"),   NULL },
    { VD_FMT_LIT("bool"),    NULL },
    { VD_FMT_LIT("char"),    NULL },
    { VD_FMT_LIT("void"),    NULL },
    { VD_FMT_LIT("pathu32"), vd_fmt__pathu32_to_str },
    { VD_FMT_LIT("null"),    vd_fmt__null_to_str },
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

                    nwrite += _vd_g.lut[i].p(out + nwrite, available, &args);
                    found = 1;
                    break;
                }

                i++;
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

#endif
#endif