#include "stdafx.h"
#include "AModule_API.h"

static LIST_HEAD(g_module);
static AOption *g_option = NULL;
static BOOL g_inited = FALSE;
static long g_index = 0;

AMODULE_API struct list_head*
AModuleList() {
	return &g_module;
}
#define list_for_all_AModule(pos)  list_for_each2(pos, &g_module, AModule, global_entry)


static int  AModuleInitNull(AOption *global_option, AOption *module_option, int first) { return 0; }
static void AModuleExitNull(int inited) { }
static int  AModuleCreateNull(AObject **object, AObject *parent, AOption *option) { return -ENOSYS; }
static void AModuleReleaseNull(AObject *object) { }
static int  AObjectProbeNull(AObject *other, AMessage *msg, AOption *option) { return -ENOSYS; }

AMODULE_API int
AModuleRegister(AModule *module)
{
	if (module->init == NULL) module->init = &AModuleInitNull;
	if (module->exit == NULL) module->exit = &AModuleExitNull;
	if (module->create == NULL) module->create = &AModuleCreateNull;
	if (module->release == NULL) module->release = &AModuleReleaseNull;
	if (module->probe == NULL) module->probe = &AObjectProbeNull;

	module->class_entry.init();
	list_for_all_AModule(pos)
	{
		if (strcasecmp(pos->class_name, module->class_name) == 0) {
			list_add_tail(&module->class_entry, &pos->class_entry);
			break;
		}
	}
	module->global_index = InterlockedAdd(&g_index, 1);
	g_module.push_back(&module->global_entry);
	TRACE("%2d: %s(%s): object size = %d.\n", module->global_index,
		module->module_name, module->class_name, module->object_size);

	// delay init() in AModuleInit()
	if (!g_inited)
		return 0;

	AOption *option = g_option->find(module->module_name);
	if ((option == NULL) && (g_option != NULL))
		option = AOptionFind3(g_option, module->class_name, module->module_name);

	int result = module->init(g_option, option, TRUE);
	if (result < 0) {
		TRACE("%s(%s) init() = %d.\n", module->module_name, module->class_name, result);
		module->exit(result);

		list_del_init(&module->global_entry);
		if (!list_empty(&module->class_entry))
			list_del_init(&module->class_entry);
	}
	return result;
}

AMODULE_API int
AModuleInit(AOption *option)
{
	if_not(g_option, option, release_s);
	BOOL first = !g_inited; g_inited = TRUE;

	list_for_all_AModule(module)
	{
		option = g_option->find(module->module_name);
		if ((option == NULL) && (g_option != NULL))
			option = AOptionFind3(g_option, module->class_name, module->module_name);

		int result = module->init(g_option, option, first);
		if (result < 0) {
			TRACE("%s(%s) reload = %d.\n", module->module_name, module->class_name, result);
		}
	}
	return 1;
}

AMODULE_API int
AModuleExit(void)
{
	while (!list_empty(&g_module)) {
		AModule *pos = list_pop_front(&g_module, AModule, global_entry);
		pos->exit(g_inited);
	}
	g_inited = FALSE;
	release_s(g_option);
	return 1;
}

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name)
{
	if ((class_name == NULL) && (module_name == NULL))
		return NULL;

	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;
		if ((module_name == NULL) || (strcasecmp(module_name, pos->module_name) == 0))
			return pos;
		if (class_name == NULL)
			continue;

		list_for_each2(class_pos, &pos->class_entry, AModule, class_entry) {
			if (strcasecmp(module_name, class_pos->module_name) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param)
{
	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;

		int result = comp(param, pos);
		if (result > 0)
			return pos;
		if (result < 0)
			return NULL;

		if (class_name == NULL)
			continue;

		list_for_each2(class_pos, &pos->class_entry, AModule, class_entry) {
			result = comp(param, class_pos);
			if (result > 0)
				return class_pos;
			if (result < 0)
				return NULL;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg, AOption *option)
{
	AModule *module = NULL;
	int score = 0;
	int ret;

	list_for_all_AModule(pos)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;

		ret = pos->probe(other, msg, option);
		if (ret > score) {
			score = ret;
			module = pos;
		}

		if (class_name == NULL)
			continue;

		list_for_each2(class_pos, &pos->class_entry, AModule, class_entry)
		{
			ret = class_pos->probe(other, msg, option);
			if (ret > score) {
				score = ret;
				module = class_pos;
			}
		}
		break;
	}
	return module;
}

//////////////////////////////////////////////////////////////////////////
AMODULE_API int
AObjectCreate(AObject **object, AObject *parent, AOption *option, const char *default_module)
{
	const char *class_name = NULL;
	const char *module_name = default_module;
	if (option == NULL) {
		;
	} else if ((option->value[0] != '\0') && (option->value[0] != '[')) {
		class_name = option->name;
		module_name = option->value;
	} else if (option->name[0] != '\0') {
		module_name = option->name;
	}
	if ((class_name == NULL) && (module_name == NULL))
		return -EINVAL;

	AModule *module = AModuleFind(class_name, module_name);
	if (module == NULL)
		return -ENOSYS;

	return AObjectCreate2(object, parent, option, module);
}

AMODULE_API int
AObjectCreate2(AObject **object, AObject *parent, AOption *option, AModule *module)
{
	if (module->object_size > 0) {
		*object = (AObject*)malloc(module->object_size);
		if (*object == NULL)
			return -ENOMEM;

		InterlockedAdd(&module->object_count, 1);
		(*object)->init(module, &AObjectFree);
	} else {
		*object = NULL;
	}

	int result = module->create(object, parent, option);
	if (result < 0)
		release_s(*object);
	return result;
}

AMODULE_API void
AObjectFree(AObject *object)
{
	object->_module->release(object);
	InterlockedAdd(&object->_module->object_count, -1);
	free(object);
}

#ifdef _WIN32
#include <direct.h>
#ifdef _WIN64
	#define DLL_BIN_OS    "Win64"
#else
	#define DLL_BIN_OS    "Win32"
#endif
#define DLL_BIN_LIB   ""
#define DLL_BIN_NAME  "dll"

#define RTLD_NOW  0
static inline void*
dlopen(const char *filename, int flag) {
	return LoadLibraryExA(filename, NULL, flag);
}
#else //_WIN32
#include <dlfcn.h>
#if defined(__LP64__) && (__LP64__)
	#define DLL_BIN_OS    "linux64"
#else
	#define DLL_BIN_OS    "linux32"
#endif
#define DLL_BIN_LIB   "lib"
#define DLL_BIN_NAME  "so"
#endif //!_WIN32

static long dlload_tid = 0;

AMODULE_API void*
dlload(const char *relative_path, const char *dll_name, BOOL relative_os_name)
{
	long cur_tid = gettid();
	long last_tid = 0;

	for (;;) {
		last_tid = InterlockedCompareExchange(&dlload_tid, cur_tid, 0);
		if ((last_tid == 0) || (last_tid == cur_tid))
			break;

		TRACE("%s: current(%d) wait other(%d) completed...\n",
			dll_name, cur_tid, last_tid);
		Sleep(10);
	}

	char cur_path[BUFSIZ];
	getcwd(cur_path, sizeof(cur_path));

	char abs_path[BUFSIZ];
#ifdef _WIN32
	DWORD len = GetModuleFileNameA(NULL, abs_path, sizeof(abs_path));
	const char *pos = strrchr(abs_path, '\\');
#else
	size_t len = readlink("/proc/self/exe", abs_path, sizeof(abs_path));
	const char *pos = strrchr(abs_path, '/');
#endif
	len = pos - abs_path + 1;
	abs_path[len] = '\0';

	if (relative_path != NULL) {
		if (relative_os_name) {
			len += snprintf(abs_path+len, sizeof(abs_path)-len,
				"%s_%s/", relative_path, DLL_BIN_OS);
		} else {
			len += snprintf(abs_path+len, sizeof(abs_path)-len,
				"%s/", relative_path);
		}
		chdir(abs_path);
	}
	snprintf(abs_path+len, sizeof(abs_path)-len,
		"%s%s.%s", DLL_BIN_LIB, dll_name, DLL_BIN_NAME);

	void *module = dlopen(abs_path, RTLD_NOW);
	if ((module == NULL) && (relative_path != NULL))
		module = dlopen(abs_path+len, RTLD_NOW);

	if (relative_path != NULL) {
		chdir(cur_path);
	}
	if (last_tid == 0) {
		InterlockedExchange(&dlload_tid, 0);
	}
	return module;
}

#if 0
AMODULE_API int
AObjectSetKVOpt(AObject *object, const ObjKV *kv, AOption *opt)
{
	if (kv->type == ObjKV_string) {
		strcpy_sz((char*)object+kv->offset, kv->size, opt?opt->value:kv->defstr);
		return 1;
	}
	if (kv->type == ObjKV_char) {
		*((char*)object+kv->offset) = opt?opt->value[0]:kv->defstr[0];
		return 1;
	}
	if (kv->type == ObjKV_object) {
		//return (AObjectCreate((AObject**)((char*)object+kv->offset), object, opt, kv->defstr) >= 0);
		AObject *child = *(AObject**)((char*)object+kv->offset);
		if ((child == NULL) && (opt == NULL))
			return 0;
		if (child == NULL) {
			if (AObjectCreate(&child, object, opt, kv->defstr) < 0)
				return 0;
			*(AObject**)((char*)object+kv->offset) = child;
			if (opt == NULL)
				return 1;
		} else {
			if (opt == NULL)
				return 0;
		}

		int count = 0;
		/*AOption *child_opt;
		list_for_each_entry(child_opt, &opt->children_list, AOption, brother_entry)
		{
			if (AObjectSetOpt(child, child_opt, child->module->kv_map) > 0)
				++count;
		}*/
		return count;
	}

	int64_t v = opt ? _atoi64(opt->value) : kv->defnum;
	switch (kv->type)
	{
	case ObjKV_int8:   *(int8_t*)((char*)object+kv->offset) = (int8_t)v; break;
	case ObjKV_int16:  *(int16_t*)((char*)object+kv->offset) = (int16_t)v; break;
	case ObjKV_int32:  *(int32_t*)((char*)object+kv->offset) = (int32_t)v; break;
	case ObjKV_int64:  *(int64_t*)((char*)object+kv->offset) = v; break;
	default: return 0;
	}
	return 1;
}

AMODULE_API int
AObjectSetKVMap(AObject *object, const ObjKV *kv_map, AOption *option, BOOL skip_unfound)
{
	int count = 0;
	for (const ObjKV *kv = kv_map; kv->name != NULL; ++kv)
	{
		AOption *opt = AOptionFind(option, kv->name);
		if (opt != NULL || !skip_unfound)
			count += AObjectSetKVOpt(object, kv, opt);
	}
	return count;
}

AMODULE_API int
AObjectSetOpt(AObject *object, AOption *opt, const ObjKV *kv_map)
{
	for (const ObjKV *kv = kv_map; kv->name != NULL; ++kv)
	{
		if (strcmp(opt->name, kv->name) == 0)
			return AObjectSetKVOpt(object, kv, opt) ? 1 : -ENOENT;
	}
	return 0;
}
#endif
