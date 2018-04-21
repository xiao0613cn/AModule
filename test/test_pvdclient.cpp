#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "test.h"
#include "../ecs/AEvent.h"
#include "../ecs/AClientSystem.h"
#include "../device_agent/device.h"
#include "../PVDClient/PvdNetCmd.h"

extern int on_event(AReceiver *r, void *p, bool preproc);

CU_TEST(test_pvd)
{
	dlload(NULL, "PVDClient");
	const char *opt_str =
	"PVDClient: {"
		"io: async_tcp {"
			"address: 192.168.30.248,"
			"port: 8101,"
			"timeout: 5,"
		"},"
		"devid: 123456,"
		"login_user: admin,"
		"login_pwd: Hb888888,"
	"}";
	AOption *opt = NULL;
	int result = AOptionDecode(&opt, opt_str, -1);

	AEntity *e = NULL;
	result = AObject::create(&e, NULL, opt, NULL);
	opt->release();
	if (e == NULL)
		return;
	AClientComponent *c; e->get(&c);

	ASystemManager *sm = ASystemManager::get();
	AEventManager *em = sm->_event_manager;
	AReceiver *r;
	em->lock();
	r = em->_sub_self(em, "on_client_opened", c, &on_event); r->_oneshot = true; r->release();
	r = em->_sub_self(em, "on_client_opened", c, &on_event); r->_oneshot = false; r->release();
	r = em->_sub_self(em, "on_client_closed", c, &on_event); r->_oneshot = true; r->release();
	r = em->_sub_self(em, "on_client_closed", c, &on_event); r->_oneshot = false; r->release();
	em->unlock();

	AEntityManager *etm = sm->_all_entities;
	etm->lock();
	etm->_push(etm, e);
	etm->unlock();

	sm->lock();
	sm->_regist(e); e->release();
	sm->unlock();
}