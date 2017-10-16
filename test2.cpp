#include "stdafx.h"
//#include <Windows.h>
//#include <crtdbg.h>
#include "base/AModule_API.h"
#include "base/spinlock.h"
#include "ecs/AEntity.h"
#include "ecs/AEvent.h"
#include "ecs/ASystem.h"
#include "ecs/AInOutComponent.h"


static int on_event(void *user, const char *name, void *p, bool preproc) {
	TRACE("user = %p, name = %s, p = %p, preproc = %d.\n", user, name, p, preproc);
	return 1;
}

int main()
{
	AModuleInit(NULL);
	AThreadBegin(NULL, NULL, 1000);

	const char *opt_str =
	"PVDClient: {"
		"io: async_tcp {"
			"address: 192.168.40.86,"
			"port: 8101,"
			"timeout: 5,"
		"},"
		"username: admin,"
		"password: 888888,"
	"}";
	AOption *opt = NULL;
	int result = AOptionDecode(&opt, opt_str, -1);

	AEntity2 *e = NULL;
	result = AObject::create(&e, NULL, opt, NULL);
	opt->release();

	AEntity2 *mqtt = NULL;
	AOptionDecode(&opt, "MQTTClient: { io: async_tcp { address: test.mosquitto.org, port: 1883, }, }", -1);
	result = AObject::create(&mqtt, NULL, opt, NULL);
	opt->release();

	AEventManager em; em.init();
	em._sub_const("on_client_opened", false, e, &on_event);
	em._sub_const("on_client_opened", true, e, &on_event);
	em._sub_const("on_client_opened", true, mqtt, &on_event);
	em._sub_const("on_client_closed", true, e, &on_event);
	em._sub_const("on_client_closed", true, mqtt, &on_event);
	em._sub_const("on_client_closed", false, e, &on_event);

	ASystemManager sm; sm.init();
	sm._event_manager = &em;

	sm._regist(e);
	sm._regist(mqtt);
	for (int ix = 0; ix < 400; ++ix) {
		sm.check_allsys(&sm, GetTickCount());
		if (ix == 200) {
			AInOutComponent *c = e->_get<AInOutComponent>();
			c->_input_end(1);
		}
		::Sleep(100);
	}
	sm._unregist(e); e->release();

	for (;;) {
		sm.check_allsys(&sm, GetTickCount());
		::Sleep(10);
	}
	sm._unregist(mqtt); mqtt->release();

	_CrtDumpMemoryLeaks();
	return 0;
}
