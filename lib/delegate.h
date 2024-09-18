#ifndef VD_DELEGATE_H
#define VD_DELEGATE_H
#include "common.h"
#include "array.h"

#define VD_HOOK(name) name##Hook
#define VD_HOOK_INIT(hook, allocator) VD_ARRAY_INIT(hook.list, allocator)
#define VD_HOOK_DEINIT(hook) VD_ARRAY_DEINIT(hook.list)

#define VD_HOOK_SUBSCRIBE(hook, fptr, usrdatavalue)      \
	{                                                    \
		unsigned int _lastidx = VD_ARRAY_LEN(hook.list); \
		VD_ARRAY_PUSH(hook.list);                        \
		hook.list[_lastidx].func = fptr;                 \
		hook.list[_lastidx].usrdata = usrdatavalue;      \
	}                                                    \

#define VD_HOOK_UNSUBSCRIBE(hook, fptr)                     \
	{                                                       \
		int _f = -1;                                        \
		for (int i = 0; i < VD_ARRAY_LEN(hook.list); ++i) { \
			if (hook.list[i].func == fptr) {                \
				_f = i;                                     \
				break;                                      \
			}                                               \
		}                                                   \
		if (_f) {                                           \
			VD_ARRAY_DELSWAP(hook.list, _f);                \
		}                                                   \
	}                                                       \

#define VD_HOOK_INVOKE(hook, ...)                                   \
	{                                                               \
		for (int _i = 0; _i < VD_ARRAY_LEN(hook.list); ++_i) {      \
			hook.list[_i].func(__VA_ARGS__, hook.list[_i].usrdata); \
		}                                                           \
	}                                                               \

#define VD_HOOK_INVOKE_RET(hook, ret, ...)                                 \
	{                                                                      \
		for (int _i = 0; _i < VD_ARRAY_LEN(hook.list); ++_i) {             \
			ret = hook.list[_i].func(__VA_ARGS__, hook.list[_i].usrdata);  \
		}                                                                  \
	}                                                                      \

#define VD_HOOK_INVOKE_RET_CALLBACK(hook, retcallback, retcallbackusrdata, ...)                       \
	{                                                                                                 \
		for (int _i = 0; _i < VD_ARRAY_LEN(hook.list); ++_i) {                                        \
			retcallback(hook.list[_i].func(__VA_ARGS__, hook.list[_i].usrdata), retcallbackusrdata);  \
		}                                                                                             \
	}                                                                                                 \

#define VD_CALLBACK(name) name##Callback

#define VD_CALLBACK_INVOKE(callback, ...) callback.entry.func(__VA_ARGS__, callback.entry.usrdata)

#define VD_DELEGATE_DECLARE_1(name, rettype, ...)       \
	typedef rettype (name)(__VA_ARGS__, void *usrdata); \
	typedef struct {                                    \
		name *func;                                     \
		void *usrdata;                                  \
	} name##Entry;										\
	typedef struct {                                    \
		name##Entry *list;								\
	} VD_HOOK(name);                                    \
	typedef struct {                                    \
		name##Entry entry;								\
	} VD_CALLBACK(name);                                \


#define VD_DELEGATE_DECLARE_PARAMS1_VOID(name, param1_type, param1_name) \
	VD_DELEGATE_DECLARE_1(name, void, param1_type param1_name);
#define VD_DELEGATE_DECLARE_PARAMS2_VOID(name, param1_type, param1_name, param2_type, param2_name) \
	VD_DELEGATE_DECLARE_1(name, void, param1_type param1_name, param2_type param2_name);
#define VD_DELEGATE_DECLARE_PARAMS1_RET(name, rettype, param1_type, param1_name) \
	VD_DELEGATE_DECLARE_1(name, rettype, param1_type param1_name);

#endif // !VD_DELEGATE_H
