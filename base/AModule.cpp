#include "stdafx.h"
#include "AModule_API.h"

static LIST_HEAD(g_module);
static AOption *g_option = NULL;
static BOOL g_inited = FALSE;
static long g_index = 0;

AMODULE_API struct list_head*
AModuleList()
{
	return &g_module;
}


static int  AModuleInitNull(AOption *global_option, AOption *module_option, int first) { return 0; }
static void AModuleExitNull(int inited) { }
static int  AModuleCreateNull(AObject **object, AObject *parent, AOption *option) { return -ENOSYS; }
static void AModuleReleaseNull(AObject *object) { }
static int  AObjectProbeNull(AObject *other, AMessage *msg) { return -ENOSYS; }

AMODULE_API int
AModuleRegister(AModule *module)
{
	if (module->init == NULL) module->init = &AModuleInitNull;
	if (module->exit == NULL) module->exit = &AModuleExitNull;
	if (module->create == NULL) module->create = &AModuleCreateNull;
	if (module->release == NULL) module->release = &AModuleReleaseNull;
	if (module->probe == NULL) module->probe = &AObjectProbeNull;

	list_add_tail(&module->global_entry, &g_module);
	module->global_index = InterlockedAdd(&g_index, 1);

	INIT_LIST_HEAD(&module->class_entry);
	module->class_index = 0;
	module->class_count = 0;

	list_for_each2(pos, &g_module, AModule, global_entry)
	{
		if (strcasecmp(pos->class_name, module->class_name) == 0) {
			list_add_tail(&module->class_entry, &pos->class_entry);
			module->class_index = InterlockedAdd(&pos->class_count, 1);
			break;
		}
	}

	if (!g_inited)
		return 0;

	AOption *option = AOptionFind(g_option, module->module_name);
	if ((option == NULL) && (g_option != NULL))
		option = AOptionFind3(&g_option->children_list, module->class_name, module->module_name);

	int result = module->init(g_option, option, TRUE);
	if (result < 0) {
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
	release_s(g_option, AOptionRelease, NULL);
	g_option = option;
	BOOL first = !g_inited;
	if (!g_inited)
		g_inited = TRUE;

	list_for_each2(module, &g_module, AModule, global_entry)
	{
		option = AOptionFind(g_option, module->module_name);
		if ((option == NULL) && (g_option != NULL))
			option = AOptionFind3(&g_option->children_list, module->class_name, module->module_name);

		int result = module->init(g_option, option, first);
		if (result < 0) {
			TRACE("module(%s) reload config failed = 0x%X.\n", module->module_name, -result);
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
	release_s(g_option, AOptionRelease, NULL);
	return 1;
}

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name)
{
	list_for_each2(pos, &g_module, AModule, global_entry)
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
	list_for_each2(pos, &g_module, AModule, global_entry)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;
		if (comp(param, pos) == 0)
			return pos;
		if (class_name == NULL)
			continue;

		list_for_each2(class_pos, &pos->class_entry, AModule, class_entry) {
			if (comp(param, class_pos) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleProbe(const char *class_name, AObject *other, AMessage *msg)
{
	AModule *module = NULL;
	int score = 0;
	int ret;

	list_for_each2(pos, &g_module, AModule, global_entry)
	{
		if ((class_name != NULL) && (strcasecmp(class_name, pos->class_name) != 0))
			continue;

		ret = pos->probe(other, msg);
		if (ret > score) {
			score = ret;
			module = pos;
		}

		if (class_name == NULL)
			continue;

		list_for_each2(class_pos, &pos->class_entry, AModule, class_entry)
		{
			ret = class_pos->probe(other, msg);
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

		(*object)->init(module);
		(*object)->_release = &AObjectFree;
	} else {
		*object = NULL;
	}

	int result = module->create(object, parent, option);
	if (result < 0)
		release_s(*object, AObjectRelease, NULL);
	return result;
}

AMODULE_API void
AObjectFree(AObject *object)
{
	object->_module->release(object);
	free(object);
}

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

