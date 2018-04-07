#include "../stdafx.h"
#include "../base/AModule_API.h"

#include "../base/spinlock.h"
#include "../ecs/AEntity.h"
#include "../ecs/AEvent.h"
#include "../ecs/ASystem.h"
#include "../ecs/AInOutComponent.h"
#include "../ecs/AClientSystem.h"
#include "../iot/MQTTComponent.h"
#include "../http/AModule_HttpSession.h"
#include "test.h"

struct TestCom : public AEntity {
	HttpParserCompenont _http;
	MqttComponent _mqtt;
};

static int test_com_create(AObject **object, AObject *parent, AOption *option)
{
	TestCom *e = (TestCom*)*object;
	e->init();
	e->_http.init(e->_http.name()); e->push(&e->_http);
	e->_mqtt.init(e->_mqtt.name()); e->push(&e->_mqtt);
	return 1;
}

static void test_com_release(AObject *object)
{
	TestCom *e = (TestCom*)object;
	e->pop(&e->_http);
	e->pop(&e->_mqtt);
	e->exit();
}

static int test_com_open(AClientComponent *c)
{
	TestCom *e = (TestCom*)c->_entity;
	return -1;
}

static int test_com_close(AClientComponent *c)
{
	TestCom *e = (TestCom*)c->_entity;
	c->_auto_reopen = false;
	c->_main_abort = true;
	return -1;
}

static AModule test_com_module = {
	"test_com_module",
	"test_com_module",
	sizeof(TestCom),
	NULL, NULL,
	&test_com_create,
	&test_com_release,
};
static int reg_test_com = AModuleRegister(&test_com_module);

CU_TEST(test_add_com)
{
	AEntity *e = NULL;
	AObject::create2(&e, NULL, NULL, &test_com_module);

	AEntityManager *em = AEntityManager::get();
	AInOutComponent *c1 = em->add_com<AInOutComponent>(e, NULL);
	AClientComponent *c2 = em->add_com<AClientComponent>(e, NULL);
	c2->open = &test_com_open;
	c2->close = &test_com_close;
	c2->_owner_thread = true;

	em->lock();
	em->_push(em, e);
	em->unlock();
	e->release();
}
