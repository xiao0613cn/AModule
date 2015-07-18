#include "stdafx.h"
#include "AModule.h"

static LIST_HEAD(module_list);
static AOption *g_option = NULL;

void AModuleRegister(AModule *module)
{
	AModule *pos;
	INIT_LIST_HEAD(&module->class_list);
	list_for_each_entry(pos, &module_list, AModule, global_entry)
	{
		if (_stricmp(module->class_name, pos->class_name) == 0) {
			list_add(&module->class_list, &pos->class_list);
			break;
		}
	}
	list_add_tail(&module->global_entry, &module_list);

	long result = 0;
	if (module->init != NULL) {
		result = module->init(g_option);
		if (result < 0) {
			list_del_init(&module->global_entry);
			list_del_init(&module->class_list);
		}
	}
	return result;
}

long AModuleInitAll(AOption *option)
{
	g_option = option;
	AModule *pos;
	list_for_each_entry(pos, &module_list, AModule, global_entry)
	{
		if ((pos->init != NULL) && (pos->init(option) < 0)) {
			if (pos->exit != NULL)
				pos->exit();
		}
	}
	return 1;
}

long AModuleExitAll(void)
{
	AModule *pos;
	list_for_each_entry(pos, &module_list, AModule, global_entry)
	{
		if (pos->exit != NULL)
			pos->exit();
	}
	release_s(g_option, AOptionRelease, NULL);
	return 1;
}

AModule* AModuleFind(const char *class_name, const char *module_name)
{
	AModule *pos;
	list_for_each_entry(pos, &module_list, AModule, global_entry)
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

AModule* AModuleEnum(const char *class_name, long(*comp)(void*,AModule*), void *param)
{
	AModule *pos;
	list_for_each_entry(pos, &module_list, AModule, global_entry)
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

AModule* AModuleProbe(const char *class_name, AObject *other, AMessage *msg)
{
	AModule *module = NULL;
	long score = -1;
	long ret;

	AModule *pos;
	list_for_each_entry(pos, &module_list, AModule, global_entry)
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
void AObjectInit(AObject *object, AModule *module)
{
	InterlockedExchange(&object->count, 1);
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

long AObjectCreate(AObject **object, AObject *parent, AOption *option, const char *default_module)
{
	const char *module_name = NULL;
	if (option != NULL) {
		if (option->value[0] != '\0')
			module_name = option->value;
		else
			module_name = option->name;
	} else {
		module_name = default_module;
	}
	if (module_name == NULL)
		return -EINVAL;

	AModule *module = AModuleFind(NULL, module_name);
	if (module == NULL)
		return -ENOSYS;

	long result = module->create(object, parent, option);
	if (result < 0)
		release_s(*object, AObjectRelease, NULL);
	return result;
}
