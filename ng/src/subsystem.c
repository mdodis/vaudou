#define VD_INTERNAL_SOURCE_FILE 1
#include "subsystem.h"

int vd_susbystem_manager_init(VD_SubsytemManager *manager)
{
	array_init(manager->subsystems, vd_memory_get_system_allocator());
	return 0;
}

int vd_susbystem_manager_deinit(VD_SubsytemManager *manager)
{
	array_deinit(manager->subsystems);
	return 0;
}