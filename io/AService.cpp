#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


AMODULE_API int
AServicePreStartChains(AService *service, AOption *option, BOOL create_chains)
{
	if (service->save_option) {
		assert(service->svc_option == NULL);
		service->svc_option = AOptionClone(option, NULL);
		if (service->svc_option == NULL)
			return -ENOMEM;
		option = service->svc_option;
	} else {
		service->svc_option = option;
	}

	if (create_chains) {
		AOption *services_list = option->find("services");
	if (services_list != NULL)
		list_for_AOption(svc_opt, services_list)
	{
		AService *svc = NULL;
		int result = AObject::create(&svc, service, svc_opt, svc_opt->value);
		if (result < 0) {
			TRACE("service(%s,%s) create()= %d.\n", svc_opt->name, svc_opt->value, result);
			continue;
		}
		svc->sysmng = service->sysmng;
		svc->parent = service;

		if ((svc->peer_module != NULL)
			&& (strcasecmp(svc->peer_module->class_name, "AEntity") != 0)) {
				TRACE("service(%s,%s) peer type(%s,%s) maybe error, require AEntity!.\n",
					svc_opt->name, svc_opt->value,
					svc->peer_module->class_name, svc->peer_module->module_name);
		}
		result = AServicePreStartChains(svc, svc_opt, create_chains);
		if (result < 0) {
			release_s(svc);
			continue;
		}
		service->children_list.push_back(&svc->brother_entry);
	} }

	if (service->require_child && service->children_list.empty()) {
		TRACE("service(%s) require children list.\n", service->_module->module_name);
		return -EINVAL;
	}

	int result = 0;
	if (service->start != NULL)
		result = service->start(service, option);
	TRACE("service(%s) start() = %d.\n", service->_module->module_name, result);
	return result;
}

AMODULE_API int
AServicePostStartChains(AService *service)
{
	if (service->post_start) {
		int result = service->start(service, NULL);
		if (result < 0) {
			TRACE("service(%s) post start() = %d.\n", service->_module->module_name, result);
			return result;
		}
	}
	list_for_AService(svc, service) {
		AServicePostStartChains(svc);
	}
	return 0;
}

AMODULE_API int
AServiceStart(AService *service, AOption *option, BOOL create_chains)
{
	int result = AServicePreStartChains(service, option, create_chains);
	if (result >= 0) {
		result = AServicePostStartChains(service);
	}
	if (result < 0) {
		AServiceStop(service, create_chains);
	}
	return result;
}

AMODULE_API void
AServiceStop(AService *service, BOOL clean_chains)
{
	if (service->stop != NULL)
		service->stop(service);
	if (clean_chains)
		while (!service->children_list.empty())
		{
			AService *svc = list_first_entry(&service->children_list, AService, brother_entry);
			AServiceStop(svc, clean_chains);
			svc->brother_entry.leave();
			release_s(svc);
		}
	else list_for_AService(svc, service)
		{
			AServiceStop(svc, clean_chains);
		}
}

AMODULE_API AService*
AServiceProbe(AService *server, AObject *object, AMessage *msg)
{
	AService *service = NULL;
	int score = -1;
	list_for_AService(svc, server)
	{
		int result = svc->_module->probe(object, msg, svc->svc_option);
		if ((result < 0) && (svc->peer_module != NULL)) {
			result = svc->peer_module->probe(object, msg, svc->svc_option);
		}
		if (result > score) {
			service = svc;
			score = result;
		}
	}
	return service;
}
