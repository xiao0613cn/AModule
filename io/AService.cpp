#include "stdafx.h"
#include "../ecs/AServiceComponent.h"

static AServiceComponent* AServiceFind(AServiceComponent *server, const char *svc_name, BOOL find_in_chains);

static int
AServiceCreateChains(AServiceComponent *server, AOption *option, BOOL create_chains)
{
	assert(!server->_running);
	if (server->_svc_option == NULL) {
		if (server->_save_option) {
			server->_svc_option = AOptionClone(option, NULL);
			if (server->_svc_option == NULL)
				return -ENOMEM;
			option = server->_svc_option;
		} else {
			server->_svc_option = option;
		}
	}

	AOption *services_opt = option->find("services");
	if (create_chains && (services_opt != NULL)) {
		list_for_AOption(svc_opt, services_opt)
	{
		AServiceComponent *svc = AServiceFind(server, svc_opt->value[0]?svc_opt->value:svc_opt->name, FALSE);
		if (svc != NULL) { // already exist
			continue;
		}
		AEntity *e = NULL;
		int result = AObject::create(&e, server->_entity, svc_opt, NULL);
		if (result < 0) {
			TRACE("server(%s): create service(%s,%s) = %d.\n",
				server->_entity->_module->module_name,
				svc_opt->name, svc_opt->value, result);
			continue;
		}
		if (e->get(&svc) == NULL) {
			TRACE("%s(%s): has not AServiceComponent..\n",
				e->_module->class_name, e->_module->module_name);
			e->release();
			continue;
		}
		svc->_sysmng = server->_sysmng;
		if (svc->_parent == NULL) {
			svc->_parent = server;
			server->_entity->addref();
		}
		server->_children_list.push_back(&svc->_brother_entry);

		AServiceCreateChains(svc, svc_opt, create_chains);
	} }

	if (server->_require_child && server->_children_list.empty()) {
		TRACE("server(%s): require children list.\n", server->_entity->_module->module_name);
		return -EINVAL;
	}
	return -1;
}

static int
AServicePreStartChains(AServiceComponent *server, AOption *option, BOOL create_chains)
{
	assert(!server->_running);
	if (server->_svc_option == NULL) {
		if (server->_save_option) {
			server->_svc_option = AOptionClone(option, NULL);
			if (server->_svc_option == NULL)
				return -ENOMEM;
			option = server->_svc_option;
		} else {
			server->_svc_option = option;
		}
	}

	AOption *services_opt = option->find("services");
	if (create_chains && (services_opt != NULL)) {
		list_for_AOption(svc_opt, services_opt)
	{
		AServiceComponent *svc = AServiceFind(server, svc_opt->value[0]?svc_opt->value:svc_opt->name, FALSE);
		if (svc != NULL) { // already exist
			continue;
		}

		AEntity *e = NULL;
		int result = AObject::create(&e, server->_entity, svc_opt, NULL);
		if (result < 0) {
			TRACE("server(%s): create service(%s,%s) = %d.\n",
				server->_entity->_module->module_name,
				svc_opt->name, svc_opt->value, result);
			continue;
		}
		if (e->get(&svc) == NULL) {
			TRACE("entity(%s): has not AServiceComponent..\n",
				e->_module->module_name);
			e->release();
			continue;
		}
		server->_children_list.push_back(&svc->_brother_entry);
	} }

	if (server->_require_child && server->_children_list.empty()) {
		TRACE("server(%s): require children list.\n", server->_entity->_module->module_name);
		return -EINVAL;
	}
	list_for_AService(svc, server) {
		AOption *svc_opt = NULL;
		if (services_opt != NULL) {
			svc_opt = services_opt->find(svc->_entity->_module->module_name);
			if (svc_opt == NULL)
				svc_opt = AOptionFind3(services_opt, svc->_entity->_module->class_name, svc->_entity->_module->module_name);
		}

		svc->_sysmng = server->_sysmng;
		if (svc->_parent == NULL) {
			svc->_parent = server;
			server->_entity->addref();
		}

		int result = AServicePreStartChains(svc, svc_opt, create_chains);
		if (result < 0)
			return result;
	}

	int result = 0;
	server->_running = TRUE;
	if (server->start != NULL)
		result = server->start(server, FALSE);
	TRACE("server(%s): start() = %d.\n", server->_entity->_module->module_name, result);
	return result;
}

static int
AServicePostStartChains(AServiceComponent *server)
{
	assert(server->_running);
	if (server->_post_start) {
		int result = server->start(server, TRUE);
		if (result < 0) {
			TRACE("server(%s): post start() = %d.\n", server->_entity->_module->module_name, result);
			return result;
		}
	}
	list_for_AService(svc, server) {
		AServicePostStartChains(svc);
	}
	return 0;
}

static void
AServiceStop(AServiceComponent *server, BOOL clean_chains)
{
	server->_running = FALSE;
	if (server->stop != NULL)
		server->stop(server);
	if (clean_chains) {
		while (!server->_children_list.empty())
		{
			AServiceComponent *svc = list_first_entry(&server->_children_list, AServiceComponent, _brother_entry);
			AServiceStop(svc, clean_chains);
			svc->_brother_entry.leave();
			svc->_entity->release();
		}
	} else {
		list_for_AService(svc, server) {
			AServiceStop(svc, clean_chains);
		}
	}
}

static int
AServiceStart(AServiceComponent *server, AOption *option, BOOL create_chains)
{
	int result = AServicePreStartChains(server, option, create_chains);
	if (result >= 0) {
		result = AServicePostStartChains(server);
	}
	if (result < 0) {
		AServiceStop(server, FALSE);
	}
	return result;
}

static AServiceComponent*
AServiceProbe(AServiceComponent *server, AObject *object, AMessage *msg)
{
	AServiceComponent *service = NULL;
	int score = -1;
	list_for_AService(svc, server)
	{
		int result = svc->_entity->_module->probe(svc->_entity, object, msg);
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

static AServiceComponent*
AServiceFind(AServiceComponent *server, const char *svc_name, BOOL find_in_chains)
{
	if (strcasecmp(server->_entity->_module->module_name, svc_name) == 0)
		return server;
	list_for_AService(svc, server)
	{
		if (find_in_chains) {
			AServiceComponent *s = AServiceFind(svc, svc_name, find_in_chains);
			if (s != NULL)
				return s;
		} else {
			if (strcasecmp(svc->_entity->_module->module_name, svc_name) == 0)
				return svc;
		}
	}
	return NULL;
}

static int svc_com_run_null(AServiceComponent *svc, AObject *peer) {
	return -ENOSYS;
}

static int svc_com_create(AObject **object, AObject *parent, AOption *options)
{
	AServiceComponent *svc = (AServiceComponent*)*object;
	svc->init2();
	svc->run   = &svc_com_run_null;
	return 1;
}

AServiceComponentModule SCM = { {
	AServiceComponent::name(),
	AServiceComponent::name(),
	sizeof(AServiceComponent),
	NULL, NULL,
	&svc_com_create,
},
	&AServiceStart,
	&AServiceStop,
	&AServiceProbe,
	&AServiceFind,
};
static int reg_scm = AModuleRegister(&SCM.module);
