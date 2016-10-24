#include "stdafx.h"
#include "AModule_API.h"

static LIST_HEAD(g_module);
static AOption *g_option = NULL;

AMODULE_API int
AModuleRegister(AModule *module)
{
	list_add_tail(&module->global_entry, &g_module);
	INIT_LIST_HEAD(&module->class_entry);

	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if (_stricmp(pos->class_name, module->class_name) == 0) {
			list_add_tail(&module->class_entry, &pos->class_entry);
			break;
		}
	}

	int result = 1;
	if (module->init != NULL) {
		result = module->init(g_option);

		if (result < 0) {
			if (module->exit != NULL)
				module->exit();

			list_del_init(&module->global_entry);
			if (!list_empty(&module->class_entry))
				list_del_init(&module->class_entry);
		}
	}
	return result;
}

AMODULE_API int
AModuleInitOption(AOption *option)
{
	g_option = option;
	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if ((pos->init != NULL) && (pos->init(option) < 0)) {
			if (pos->exit != NULL)
				pos->exit();
		}
	}
	return 1;
}

AMODULE_API int
AModuleExit(void)
{
	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if (pos->exit != NULL)
			pos->exit();
	}
	release_s(g_option, AOptionRelease, NULL);
	return 1;
}

AMODULE_API AModule*
AModuleFind(const char *class_name, const char *module_name)
{
	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if ((class_name != NULL) && (_stricmp(class_name, pos->class_name) != 0))
			continue;
		if ((module_name == NULL) || (_stricmp(module_name, pos->module_name) == 0))
			return pos;
		if (class_name == NULL)
			continue;

		AModule *class_pos;
		list_for_each_entry(class_pos, &pos->class_entry, AModule, class_entry)
		{
			if (_stricmp(module_name, class_pos->module_name) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
AModuleEnum(const char *class_name, int(*comp)(void*,AModule*), void *param)
{
	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if ((class_name != NULL) && (_stricmp(class_name, pos->class_name) != 0))
			continue;
		if (comp(param, pos) == 0)
			return pos;
		if (class_name == NULL)
			continue;

		AModule *class_pos;
		list_for_each_entry(class_pos, &pos->class_entry, AModule, class_entry)
		{
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
	int score = -1;
	int ret;

	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if ((class_name != NULL) && (_stricmp(class_name, pos->class_name) != 0))
			continue;
		if (pos->probe != NULL) {
			ret = pos->probe(other, msg);
			if (ret > score) {
				score = ret;
				module = pos;
			}
		}
		if (class_name == NULL)
			continue;

		AModule *class_pos;
		list_for_each_entry(class_pos, &pos->class_entry, AModule, class_entry)
		{
			if (pos->probe != NULL) {
				ret = class_pos->probe(other, msg);
				if (ret > score) {
					score = ret;
					module = class_pos;
				}
			}
		}
		break;
	}
	return module;
}

//////////////////////////////////////////////////////////////////////////
AMODULE_API void
AObjectInit(AObject *object, AModule *module)
{
	object->refcount = 1;
	object->release = module->release;
	object->extend = NULL;
	object->module = module;
	object->reqix_count = module->reqix_count;

	object->open = module->open;
	object->setopt = module->setopt;
	object->getopt = module->getopt;
	object->request = module->request;
	object->cancel = module->cancel;
	object->close = module->close;
}

AMODULE_API int
AObjectCreate(AObject **object, AObject *parent, AOption *option, const char *default_module)
{
	const char *class_name = NULL;
	const char *module_name = NULL;
	if (option != NULL) {
		class_name = option->name;
		if (option->value[0] != '\0')
			module_name = option->value;
		else
			module_name = option->name;
	} else {
		module_name = default_module;
	}
	if ((class_name == NULL) && (module_name == NULL))
		return -EINVAL;

	AModule *module = AModuleFind(class_name, module_name);
	if (module == NULL)
		return -ENOSYS;

	if (module->object_size > 0) {
		*object = (AObject*)malloc(module->object_size);
		if (*object == NULL)
			return -ENOMEM;

		AObjectInit(*object, module);
	} else {
		*object = NULL;
	}

	int result = module->create(object, parent, option);
	if (result < 0)
		release_s(*object, AObjectRelease, NULL);
	return result;
}
