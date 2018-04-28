#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "test.h"
#include "../ecs/AEvent.h"
#include "../ecs/AClientSystem.h"

extern int on_event(AReceiver *r, void *p, bool preproc);

CU_TEST(test_mqtt)
{
	dlload(NULL, "mqtt_client");
	const char *str;
	str = "MQTTClient: { io: io_openssl { "
		"io: async_tcp { address: test.mosquitto.org, port: 8883, },"
		"}, }";
	//str = "MQTTClient: { io: async_tcp { address: 60.210.40.196, port: 25102 } }";

	AOption *opt = NULL;
	AOptionDecode(&opt, str, -1);

	AEntity *mqtt = NULL;
	int result = AObject::create(&mqtt, NULL, opt, NULL);
	opt->release();
	if (mqtt == NULL)
		return;

	AClientComponent *c; mqtt->get(&c);
	ASystemManager *SM = ASystemManager::get();
	AEventManager *EV = SM->_event_manager;
	AReceiver *r;
	EV->lock();
	r = EV->_sub_self(EV, "on_client_opened", c, &on_event); r->_oneshot = true; r->release();
	r = EV->_sub_self(EV, "on_client_opened", c, &on_event); r->_oneshot = false; r->release();
	r = EV->_sub_self(EV, "on_client_closed", c, &on_event); r->_oneshot = true; r->release();
	r = EV->_sub_self(EV, "on_client_closed", c, &on_event); r->_oneshot = false; r->release();
	EV->unlock();

	AEntityManager *EM = SM->_all_entities;
	EM->lock();
	EM->_push(EM, mqtt);
	EM->unlock();

	SM->lock();
	SM->_regist(mqtt);
	SM->unlock();
	mqtt->release();
}
