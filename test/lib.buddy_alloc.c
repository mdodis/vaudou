#define VD_BUDDY_ALLOC_IMPLEMENTATION
#include "vd_buddy_alloc.h"
#include "utest.h"

UTEST(buddy_alloc, test_basic_invocation)
{
	VD_BuddyAlloc allocator;
	vd_buddy_alloc_init(&allocator, vd_buddy_alloc_default_palloc, 0, 1023, 16);

	void *p = vd_buddy_alloc_realloc(&allocator, 0, 16);

	EXPECT_NE(p, 0);

	vd_buddy_alloc_deinit(&allocator);
}