#ifndef VD_SUBSYSTEM_H
#define VD_SUBSYSTEM_H
#include "vd_common.h"
#include "interface.h"
#include "array.h"

typedef struct VD_Instance VD_Instance;

typedef struct VD__tag_SubsytemManager {
    VD_Instance             *instance;
    VD_ARRAY VD_InterfaceID *subsystems;
} VD_SubsytemManager;

#define VD_SUBSYSTEM_INIT_PROC(name)    int name(VD_Instance *instance, void *subsystem)
#define VD_SUBSYSTEM_DEINIT_PROC(name)  int name(VD_Instance *instance, void *subsystem)

typedef VD_SUBSYSTEM_INIT_PROC(VD_SubsystemInitProc);
typedef VD_SUBSYSTEM_DEINIT_PROC(VD_SusbystemDeinitProc);

typedef struct VD__tag_Subsystem {
    VD_InterfaceID           id;
    VD_SubsystemInitProc    *init;
    VD_SusbystemDeinitProc  *deinit;
} VD_Subsytem;

int vd_susbystem_manager_init(VD_SubsytemManager *manager);
int vd_susbystem_manager_deinit(VD_SubsytemManager *manager);

#endif //!VD_SUBSYSTEM_H