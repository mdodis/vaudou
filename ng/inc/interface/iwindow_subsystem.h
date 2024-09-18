#ifndef VD_IWINDOW_SUBSYSTEM_H
#define VD_IWINDOW_SUBSYSTEM_H
#include "common.h"
#include "subsystem.h"

#define VD_IWINDOW_SUBSYSTEM_GET_WINDOW_PLATFORM_HANDLE_PROC(name) int name(void **phandle, void *usrdata)
#define VD_IWINDOW_SUBSYSTEM_GET_WINDOW_DIM_PROC(name) int name(int *width, int *height, void *usrdata)
#define VD_IWINDOW_SUBSYSTEM_GET_WINDOW_QUIT(name) int name(void *usrdata)

typedef VD_IWINDOW_SUBSYSTEM_GET_WINDOW_PLATFORM_HANDLE_PROC(VD_IWindowSubsystemGetWindowPlatformHandleProc);
typedef VD_IWINDOW_SUBSYSTEM_GET_WINDOW_DIM_PROC(VD_IWindowSubsystemGetWindowDimProc);
typedef VD_IWINDOW_SUBSYSTEM_GET_WINDOW_QUIT(VD_IWindowSubsystemGetWindowQuitProc);

typedef struct VD__tag_IWindowSubsystem {
	VD_Subsytem										 super;
	VD_IWindowSubsystemGetWindowPlatformHandleProc	*get_window_platform_handle;
	VD_IWindowSubsystemGetWindowDimProc				*get_window_dim;
	VD_IWindowSubsystemGetWindowQuitProc			*get_window_quit;
	void											*usrdata;
} VD_IWindowSubsystem;


#endif // !VD_IWINDOW_SUBSYSTEM_H
