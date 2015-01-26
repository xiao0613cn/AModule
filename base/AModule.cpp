#include "stdafx.h"
#include "AModule.h"

static LIST_HEAD(module_list);

void AModuleRegister(AModule *module)
{
	list_add(&module->global_entry, &module_list);
}

long AModuleInitAll(void)
{
	AModule *module;
	list_for_each_entry(module, &module_list, AModule, global_entry) {
		if (module->init != NULL)
			module->init();
	}
	return 1;
}

AModule* AModuleFind(const char *class_name, const char *module_name)
{
	AModule *module = NULL;
	list_for_each_entry(module, &module_list, AModule, global_entry)
	{
		if (((class_name == NULL)
		  || (_stricmp(class_name, module->class_name) == 0))
		 && ((module_name == NULL)
		  || (_stricmp(module_name, module->module_name) == 0)))
			return module;
	}
	return NULL;
}

AModule* AModuleEnum(long(*comp)(void*,AModule*), void *param)
{
	AModule *module;
	list_for_each_entry(module, &module_list, AModule, global_entry)
	{
		if (comp(param, module) == 0)
			return module;
	}
	return NULL;
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
