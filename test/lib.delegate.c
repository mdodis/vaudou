#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "delegate.h"

VD_DELEGATE_DECLARE_PARAMS1_VOID(SimpleDelegate, int, a)

static void simple_func1(int a, void *usrdata)
{
	if (a == 1)
	{
		*((int *)usrdata) = 1;
	}
}

UTEST(delegate, test_basic_invocation)
{
	int success = 0;

	VD_HOOK(SimpleDelegate) hook = { 0 };
	VD_HOOK_INIT(hook, vd_memory_get_system_allocator());

	VD_HOOK_SUBSCRIBE(hook, simple_func1, &success);

	VD_HOOK_INVOKE(hook, 1);

	VD_HOOK_UNSUBSCRIBE(hook, simple_func1);

	VD_HOOK_DEINIT(hook);

	EXPECT_EQ(success, 1);
}

VD_DELEGATE_DECLARE_PARAMS1_RET(ReturnDelegate, int, int, a)

static int return_func1(int a, void *usrdata)
{
	if (a == 1)
	{
		return *((int *)usrdata) + 1;
	}

	return 0;
}

UTEST(delegate, test_return_invocation)
{
	VD_HOOK(ReturnDelegate) hook = { 0 };
	VD_HOOK_INIT(hook, vd_memory_get_system_allocator());

	int usrdata = 0;
	VD_HOOK_SUBSCRIBE(hook, return_func1, &usrdata);

	int ret = 0;
	VD_HOOK_INVOKE_RET(hook, ret, 1);

	VD_HOOK_UNSUBSCRIBE(hook, return_func1);
	
	VD_HOOK_DEINIT(hook);

	EXPECT_EQ(ret, 1);
}

static void sum_callback(int n, void *_result)
{
	int *result = (int *)_result;
	*result += n;
}

UTEST(delegate, test_return_callback_invocation)
{
	VD_HOOK(ReturnDelegate) hook = { 0 };
	VD_HOOK_INIT(hook, vd_memory_get_system_allocator());

	int usrdata = 0;
	VD_HOOK_SUBSCRIBE(hook, return_func1, &usrdata);
	VD_HOOK_SUBSCRIBE(hook, return_func1, &usrdata);

	int ret = 0;
	VD_HOOK_INVOKE_RET_CALLBACK(hook, sum_callback, &ret, 1);

	VD_HOOK_UNSUBSCRIBE(hook, return_func1);
	VD_HOOK_UNSUBSCRIBE(hook, return_func1);

	VD_HOOK_DEINIT(hook);

	EXPECT_EQ(ret, 2);
}

VD_DELEGATE_DECLARE_PARAMS2_VOID(Simple2Delegate, int, a, float, b)