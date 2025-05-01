#define VD_LOG_IMPLEMENTATION
#define VD_ABBREVIATIONS 1
#include "vd_log.h"
#include "utest.h"
#include "vd_common.h"
#include "arena.h"
#include "rhi/interface.h"

static VD_Log Log;
void setup_log();

#if VD_PLATFORM_WINDOWS
#include "rhi_vulkan.h"

UTEST(rhi, test_init_vulkan)
{
    setup_log();

    RHI rhi;
    rhi_vulkan_populate(&rhi);
    Arena arena = vd_arena_new(VD_MEGABYTES(2), vd_memory_get_system_allocator());

    RHResult result = rhi.initialize(&rhi, &(RHInitInfo) {
        .frame_allocator = arena.allocator,
        .extensions = {
            .debug = 1,
            .headless = 1,
        }
    });

    EXPECT_EQ(result, RH_RESULT_SUCCESS);
}
#endif

void setup_log()
{
    Log.flags = VD_LOG_WRITE_STDOUT;
    VD_LOG_SET(&Log);
    VD_LOG_RESET();
}
