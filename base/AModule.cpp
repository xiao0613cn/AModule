#include "stdafx.h"
#include "AModule_API.h"

static LIST_HEAD(g_module);
static AOption *g_option = NULL;

AMODULE_API long
amodule_register(AModule *module)
{
	AModule *pos;
	INIT_LIST_HEAD(&module->class_list);
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if (_stricmp(pos->class_name, module->class_name) == 0) {
			list_add_tail(&module->class_list, &pos->class_list);
			break;
		}
	}
	list_add_tail(&module->global_entry, &g_module);

	long result = 1;
	if (module->init != NULL) {
		result = module->init(g_option);

		if (result < 0) {
			if (module->exit != NULL)
				module->exit();

			list_del_init(&module->global_entry);
			if (!list_empty(&module->class_list))
				list_del_init(&module->class_list);
		}
	}
	return result;
}

AMODULE_API long
amodule_init_option(AOption *option)
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

AMODULE_API long
amodule_exit(void)
{
	AModule *pos;
	list_for_each_entry(pos, &g_module, AModule, global_entry)
	{
		if (pos->exit != NULL)
			pos->exit();
	}
	release_s(g_option, aoption_release, NULL);
	return 1;
}

AMODULE_API AModule*
amodule_find(const char *class_name, const char *module_name)
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
		list_for_each_entry(class_pos, &pos->class_list, AModule, class_list)
		{
			if (_stricmp(module_name, class_pos->module_name) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
amodule_enum(const char *class_name, long(*comp)(void*,AModule*), void *param)
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
		list_for_each_entry(class_pos, &pos->class_list, AModule, class_list)
		{
			if (comp(param, class_pos) == 0)
				return class_pos;
		}
		break;
	}
	return NULL;
}

AMODULE_API AModule*
amodule_probe(const char *class_name, AObject *other, AMessage *msg)
{
	AModule *module = NULL;
	long score = -1;
	long ret;

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
		list_for_each_entry(class_pos, &pos->class_list, AModule, class_list)
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
aobject_init(AObject *object, AModule *module)
{
	object->count = 1;
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

AMODULE_API long
aobject_create(AObject **object, AObject *parent, AOption *option, const char *default_module)
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

	AModule *module = amodule_find(class_name, module_name);
	if (module == NULL)
		return -ENOSYS;

	long result = module->create(object, parent, option);
	if (result < 0)
		release_s(*object, aobject_release, NULL);
	return result;
}
