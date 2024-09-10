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

#if defined(_WIN32)
#include <windows.h>
#include <stdlib.h>

_Thread_local char VD_SYSUTIL__pathbuf[MAX_PATH];

typedef struct _tag_vd_sysutil__win32_dir_iter {
    HANDLE              handle;
    int                 first_file;
    WIN32_FIND_DATAA    data;
} VD_SysUtil__Win32DirIter;
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <dirent.h>
#include <errno.h>
#endif

int vd_sysutil_get_executable_path(char *buf, unsigned int *size)
{
#if defined(_WIN32)
	// @todo: GetModuleFileNameW
	static _Thread_local char path[MAX_PATH];
	DWORD real_size = GetModuleFileNameA(NULL, path, MAX_PATH);
	
	// Either ERROR_INSUFFICIENT_BUFFER or some other error.
	if (real_size == 0) {
		return -1;
	}

	*size = real_size + 1;
	if (buf) {
		memcpy(buf, path, *size);
	}
	return 0;
#endif
#if defined(__APPLE__)
    return _NSGetExecutablePath(buf, size);
#endif
}

int vd_sysutil_dir_open(const char *path, VD_SysUtilDir *dir)
{
#if defined(_WIN32)
	int l = strlen(path);
	if (path[l - 1] == '/' || path[l - 1] == '\\') {
		l--;
	}

	if ((l + sizeof("\\*")) >= MAX_PATH) {
		return -1;
	}

	memcpy(VD_SYSUTIL__pathbuf, path, l);
	memcpy(VD_SYSUTIL__pathbuf + l, "\\*", sizeof("\\*"));

	WIN32_FIND_DATAA data;
    HANDLE hnd = FindFirstFileA(VD_SYSUTIL__pathbuf, &data);
	if (hnd == INVALID_HANDLE_VALUE) {
		return -1;
	}

    VD_SysUtil__Win32DirIter *iter = malloc(sizeof(VD_SysUtil__Win32DirIter));
	dir->handle = (void*)iter;

	iter->handle = hnd;
	iter->first_file = 1;
	iter->data = data;
    return 0;
#elif defined(__APPLE__)
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
#if defined(_WIN32)
	VD_SysUtil__Win32DirIter* iter = (VD_SysUtil__Win32DirIter*)dir->handle;
	if (iter->first_file) {
		iter->first_file = 0;
	} else {
		if (!FindNextFileA(iter->handle, &iter->data)) {
			return -1;
		}
	}

	nfo->name_len = strlen(iter->data.cFileName);
	memcpy(nfo->name, iter->data.cFileName, nfo->name_len);
	nfo->size = iter->data.nFileSizeLow;
	nfo->is_dir = (iter->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	return 0;
#elif defined(__APPLE__)
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
