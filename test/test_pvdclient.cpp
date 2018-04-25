#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "test.h"
#include "../ecs/AEvent.h"
#include "../ecs/AClientSystem.h"
#include "../device_agent/device.h"
#include "../device_agent/stream.h"

extern int on_event(AReceiver *r, void *p, bool preproc);

static int on_recv_pkt(AStreamComponent *sc, AVPacket *pkt, int result)
{
	return result;
}

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
	if (e == NULL) {
		opt->release();
		return;
	}
	AClientComponent *c; e->get(&c);

	ASystemManager *sm = ASystemManager::get();
	AEventManager *ev = sm->_event_manager;
	AReceiver *r;
	ev->lock();
	r = ev->_sub_self(ev, "on_client_opened", c, &on_event); r->_oneshot = true; r->release();
	r = ev->_sub_self(ev, "on_client_opened", c, &on_event); r->_oneshot = false; r->release();
	r = ev->_sub_self(ev, "on_client_closed", c, &on_event); r->_oneshot = true; r->release();
	r = ev->_sub_self(ev, "on_client_closed", c, &on_event); r->_oneshot = false; r->release();
	ev->unlock();

	AEntityManager *em = sm->_all_entities;
	em->lock();
	em->_push(em, e);
	em->unlock();

	sm->lock();
	sm->_regist(e);
	sm->unlock();

	AEntity *s = NULL;
	result = AObject::create2(&s, e, opt, AModuleFind(NULL,"PVDStream"));
	if (result >= 0) {
		AStreamComponent *sc; s->get(&sc);
		sc->do_recv = NULL;
		sc->on_recv = on_recv_pkt;

		em->lock();
		em->_push(em, s);
		em->unlock();
		s->release();
	}
	opt->release();
	e->release();
}