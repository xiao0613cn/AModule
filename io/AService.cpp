#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


AMODULE_API int
AServiceCreateChains(AService **svc, AService *parent, AOption *option, BOOL create_chains)
{
	return -1;
}

static int
AServicePreStartChains(AService *service, AOption *option, BOOL create_chains)
{
	assert(!service->_running);
	if (service->_svc_option == NULL) {
		if (service->_save_option) {
			service->_svc_option = AOptionClone(option, NULL);
			if (service->_svc_option == NULL)
				return -ENOMEM;
			option = service->_svc_option;
		} else {
			service->_svc_option = option;
		}
	}

	AOption *services_opt = option->find("services");
	if (create_chains && (services_opt != NULL)) {
		list_for_AOption(svc_opt, services_opt)
	{
		AService *svc = NULL;
		int result = AObject::create(&svc, service, svc_opt, NULL);
		if (result < 0) {
			TRACE("service(%s,%s) create()= %d.\n", svc_opt->name, svc_opt->value, result);
			continue;
		}

		if ((svc->_peer_module != NULL)
		 && (strcasecmp(svc->_peer_module->class_name, "AEntity") != 0)) {
			TRACE("service(%s,%s) peer type(%s,%s) maybe error, require AEntity!.\n",
				svc_opt->name, svc_opt->value,
				svc->_peer_module->class_name, svc->_peer_module->module_name);
		}
		service->_children_list.push_back(&svc->_brother_entry);
	} }

	if (service->_require_child && service->_children_list.empty()) {
		TRACE("service(%s) require children list.\n", service->_module->module_name);
		return -EINVAL;
	}
	list_for_AService(svc, service) {
		AOption *svc_opt = NULL;
		if (services_opt != NULL) {
			svc_opt = services_opt->find(svc->_module->module_name);
			if (svc_opt == NULL)
				svc_opt = AOptionFind3(services_opt, svc->_module->class_name, svc->_module->module_name);
		}

		svc->_sysmng = service->_sysmng;
		addref_s(svc->_parent, service);

		int result = AServicePreStartChains(svc, svc_opt, create_chains);
		if (result < 0)
			return result;
	}

	int result = 0;
	service->_running = TRUE;
	if (service->start != NULL)
		result = service->start(service, option);
	TRACE("service(%s) start() = %d.\n", service->_module->module_name, result);
	return result;
}

static int
AServicePostStartChains(AService *service)
{
	assert(service->_running);
	if (service->_post_start) {
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
	service->_running = FALSE;
	if (service->stop != NULL)
		service->stop(service);
	if (clean_chains) {
		while (!service->_children_list.empty())
		{
			AService *svc = list_first_entry(&service->_children_list, AService, _brother_entry);
			AServiceStop(svc, clean_chains);
			svc->_brother_entry.leave();
			release_s(svc);
		}
	} else {
		list_for_AService(svc, service) {
			AServiceStop(svc, clean_chains);
		}
	}
}

AMODULE_API AService*
AServiceProbe(AService *server, AObject *object, AMessage *msg)
{
	AService *service = NULL;
	int score = -1;
	list_for_AService(svc, server)
	{
		int result = svc->_module->probe(svc, object, msg);
		if ((result < 0) && (svc->_peer_module != NULL)) {
			result = svc->_peer_module->probe(NULL, object, msg);
		}
		if (result > score) {
			service = svc;
			score = result;
		}
	}
	return service;
}

AMODULE_API AService*
AServiceFind(AService *server, const char *svc_name, BOOL find_in_chains)
{
	if (strcasecmp(server->_module->module_name, svc_name) == 0)
		return server;
	list_for_AService(svc, server)
	{
		if (find_in_chains) {
			AService *s = AServiceFind(svc, svc_name, find_in_chains);
			if (s != NULL)
				return s;
		} else {
			if (strcasecmp(svc->_module->module_name, svc_name) == 0)
				return svc;
		}
	}
	return NULL;
}
