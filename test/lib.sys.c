#define VD_INTERNAL_SOURCE_FILE 1
#include "utest.h"
#include "sys.h"

UTEST(sys, vd_get_exec_path)
{
    Arena a = arena_new(1024, vd_memory_get_system_allocator());
    str exec_path = vd_get_exec_path(&a);
    ASSERT_TRUE(exec_path.len > 0);
}