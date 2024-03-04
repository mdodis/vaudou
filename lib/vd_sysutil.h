#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_SYSUTIL_IMPLEMENTATION
#endif

#ifndef VD_SYSUTIL_H
#define VD_SYSUTIL_H

typedef struct _tag_vd_sysutil_dir_iter {
    void *handle;
} VD_SysUtilDir;

typedef struct _tag_vd_sysutil_file_info {
    unsigned int name_len;
    unsigned int size;
    unsigned int is_dir;
    char         name[256];
} VD_SysUtilFileInfo;

int vd_sysutil_get_executable_path(char *buf, unsigned int *size);
int vd_sysutil_dir_open(const char *path, VD_SysUtilDir *dir);
int vd_sysutil_dir_next(VD_SysUtilDir *dir, VD_SysUtilFileInfo *nfo);

#ifdef VD_SYSUTIL_IMPLEMENTATION
#include <string.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dirent.h>
#include <errno.h>
#endif

int vd_sysutil_get_executable_path(char *buf, unsigned int *size)
{
#if defined(__APPLE__)
    return _NSGetExecutablePath(buf, size);
#endif
}

int vd_sysutil_dir_open(const char *path, VD_SysUtilDir *dir)
{
#if defined(__APPLE__)
    DIR *handle = opendir(path);
    if (handle == NULL) {
        return -1;
    }

    dir->handle = (void*)handle;
    return 0;
#endif
}

int vd_sysutil_dir_next(VD_SysUtilDir *dir, VD_SysUtilFileInfo *nfo)
{
#if defined(__APPLE__)
    errno = 0;
    struct dirent *result = readdir((DIR*)dir->handle);
    if (result == NULL) {
        if (errno != 0) {
            return errno;
        } else {
            return -1;
        }
    }

    nfo->name_len = strlen(result->d_name);
    memcpy(nfo->name, result->d_name, nfo->name_len);
    return 0;
#endif
}

#endif
#endif
