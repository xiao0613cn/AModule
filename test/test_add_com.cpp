#include "../stdafx.h"
#include "../base/AModule_API.h"
#ifdef _WIN32
#pragma comment(lib, "..\\bin\\AModule.lib")
#endif
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
	e->_http.init(e, e->_http.name()); e->_push(&e->_http);
	e->_mqtt.init(e, e->_mqtt.name()); e->_push(&e->_mqtt);
	return 1;
}

static void test_com_release(AObject *object)
{
	TestCom *e = (TestCom*)object;
	e->_pop(&e->_http);
	e->_pop(&e->_mqtt);
	e->exit();
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
	em->_add_com(em, e, &AInOutComponent::Module()->module);
	em->_add_com(em, e, AClientComponent::Module());

	em->lock();
	em->_push(em, e);
	em->unlock();
	e->release();
}
