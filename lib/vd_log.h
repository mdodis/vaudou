// vd_log.h - A C log library
// 
// REQUIREMENTS
// - vd_fmt.h
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
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_LOG_IMPLEMENTATION
#endif

#ifndef VD_LOG_H
#define VD_LOG_H

#include "vd_fmt.h"

#include <stdio.h>
#include <assert.h>

typedef enum {
	VD_LOG_WRITE_STDOUT = 1 << 0,
	VD_LOG_MT           = 1 << 1,
    VD_LOG_WRITE_FILE   = 1 << 2,
} VD_LogFlags;

typedef struct {
	const char	*filepath;
	VD_LogFlags flags;
} VD_Log;

extern VD_Log *VD__Log;

#ifndef VD_LOG_ENABLE_DBG
#define VD_LOG_ENABLE_DBG 1
#endif

#ifndef VD_LOG_ENABLE_WRN
#define VD_LOG_ENABLE_WRN 1
#endif

#define VD_LOG_GET()  (VD__Log)
#define VD_LOG_SET(x) (VD__Log = (x))
#define VD_LOG_RESET() \
	do { \
        if (VD_LOG_GET()->flags & VD_LOG_WRITE_FILE)        \
        {                                                   \
            FILE *f = fopen(VD_LOG_GET()->filepath, "w");   \
            fflush(f);                                      \
            fclose(f);                                      \
        }                                                   \
	} while (0)

#define VD_LOG_1(fmt, ...)                                  \
	do                                                      \
	{                                                       \
        if (VD_LOG_GET()->flags & VD_LOG_WRITE_FILE)        \
        {                                                   \
            FILE *f = fopen(VD_LOG_GET()->filepath, "a");   \
            assert(f != 0);                                 \
            vd_fmt_fprintf(f, fmt, __VA_ARGS__);            \
            fflush(f);                                      \
            fclose(f);  								    \
        }                                                   \
		if (VD_LOG_GET()->flags & VD_LOG_WRITE_STDOUT)      \
		{                                                   \
			vd_fmt_fprintf(stdout, fmt, __VA_ARGS__);       \
		}                                                   \
	} while (0)

#define VD_LOG_FMT(category, fmt, ...) VD_LOG_1("[" category "/LOG]: " fmt "\n", __VA_ARGS__)
#define VD_LOG(category, message) VD_LOG_FMT(category, "%{cstr}", message)

#if VD_LOG_ENABLE_DBG
#define VD_DBG_FMT(category, fmt, ...) VD_LOG_1("[" category "/DBG]: " fmt "\n", __VA_ARGS__)
#define VD_DBG(category, message) VD_DBG_FMT(category, "%{cstr}", message)
#else
#define VD_DBG_FMT(category, fmt, ...)
#define VD_DBG(category, message)
#endif

#define VD_ERR_FMT(category, fmt, ...) VD_LOG_1("[" category "/ERR]: " fmt "\n", __VA_ARGS__)
#define VD_ERR(category, message) VD_ERR_FMT(category, "%{cstr}", message)

#if VD_LOG_ENABLE_WRN
#define VD_WRN_FMT(category, fmt, ...) VD_LOG_1("[" category "/WRN]: " fmt "\n", __VA_ARGS__)
#define VD_WRN(category, message) VD_WRN_FMT(category, "%{cstr}", message)
#else
#define VD_WRN_FMT(category, fmt, ...)
#define VD_WRN(category, message)
#endif

#endif // !VD_LOG_H


#ifdef VD_LOG_IMPLEMENTATION

VD_Log *VD__Log;

#endif
