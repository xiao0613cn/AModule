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
#include "test.h"


CuSuite *all_test_suites = CuSuiteNew();

#if defined TEST_ECHO_CLIENT
struct echo_info {
	pthread_t tid;
	int      count;
	uint64_t recv_size;
	DWORD    duration;
	char     pad[128];
};

void* echo_thread(void *p)
{
	echo_info *ei = (echo_info*)p;
	AOption *opt = NULL;
	AOptionDecode(&opt, "io: tcp { address: 192.168.40.17, port: 4444 }", -1);

	IOObject *io = NULL;
	AObject::create(&io, NULL, opt, NULL);

	AMessage msg; msg.init(opt);
	int result = io->open(&msg);
	opt->release();

	char buf[RAND_MAX] = { "echo" };
	srand(GetTickCount());
	ei->count = 0;
	ei->recv_size = 0;
	Sleep(1000);

	DWORD dwTick = GetTickCount();
	while (ei->count < 50*1000)
	{
		msg.init(ioMsgType_Block, buf, rand());
		if (msg.size == 0) msg.size = 5000;
		result = io->input(&msg);
		if (result < 0)
			break;

		msg.type = ioMsgType_Block;
		result = io->output(&msg);
		if (result < 0)
			break;
		ei->recv_size += msg.size;
		ei->count ++;
	}
	ei->duration = GetTickCount() - dwTick;

	io->release();
	return NULL;
}

CU_TEST(test_echo_client)
{
	int thread_num = 10;
	TRACE("input test thread num(default 10):\n");
	fscanf(stdin, "%d", &thread_num);
	if (thread_num == 0) thread_num = 10;

	echo_info *ei = goarrary(echo_info, thread_num);
	for (int ix = 0; ix < thread_num; ++ix) {
		pthread_create(&ei[ix].tid, NULL, &echo_thread, &ei[ix]);
	}

	int total_qps = 0;
	uint64_t total_speed = 0;
	for (int ix = 0; ix < thread_num; ++ix)
	{
		pthread_join(ei[ix].tid, NULL);
		TRACE("%2d: count = %d, diff = %.3f, recv_size = %lld KB.\n",
			ix, ei[ix].count, ei[ix].duration/1000.0, ei[ix].recv_size/1024);

		int qps = ei[ix].count*1000/ei[ix].duration; total_qps += qps;
		uint64_t speed = ei[ix].recv_size*1000/ei[ix].duration/1024; total_speed += speed;
		TRACE("%2d: qps = %d, speed = %lld KBps.\n", ix, qps, speed);
	}
	free(ei);
	TRACE("total_qps = %d, total_speed = %lld KBps.\n", total_qps, total_speed);
}
#else
CU_TEST(test_service)
{
	AOption *opt = NULL;
	AOptionDecode(&opt, "tcp_server: { port: 4444, io: io_dump { io: tcp, }, "
		"is_async: 0, services: { EchoService: {}, "
		"HttpService: { services: { HttpFileService } }, }, background: 1 }", -1);

	AService *tcp_server = NULL;
	AObject::create(&tcp_server, NULL, opt, NULL);

	ASystemManager *sm = ASystemManager::get();
	tcp_server->_sysmng = sm;
	sm->_all_services = tcp_server;

	AServiceStart(tcp_server, opt, TRUE);
	opt->release();
}

struct client_t {
	IOObject *io;
	AMessage  msg;
	AOperator asop;
	char      data[1024];
};
int c_msg_done(AMessage *msg, int result)
{
	client_t *c = container_of(msg, client_t, msg);

	if (msg->type == AMsgType_AOption) {
		((AOption*)msg->data)->delref();
	}

	ASystemManager *sm = ASystemManager::get();
	if (sm->_all_services == NULL) {
		result = -EINTR;
	} else if (c->data[0] == '\0') {
		c->msg.init(ioMsgType_Block, c->data, sprintf(c->data, "%s", "echo1234567890abcdefghijklmnopqrstuvwxyz"
			"fasdhlfjkahdlkfahslkdfhaljksdfhalksjdfhlaksjdfhlaksdjfhlakjdsfhlakjsdfheiurowieuryhqoiwuhfalkj"
			"sdbnvzxjbvljdhlfkaheiuhiou3hrihalwekjfksdjbfasbdvkljzcxhiduhfoiauwe"));
		result = c->io->input(&c->msg);
	} else if (c->msg.type == ioMsgType_Block) {
		c->msg.init(0, c->data, sizeof(c->data));
		result = c->io->output(&c->msg);
	} else {
		c->data[0] = '\0';
		result = c->asop.delay(NULL, 200);
	}
	if (result < 0) {
		TRACE("client result = %d.\n", result);
		c->io->release();
		free(c);
	}
	return result;
}
int c_asop_done(AOperator *asop, int result)
{
	client_t *c = container_of(asop, client_t, asop);
	return c->msg.done2(1);
}
CU_TEST(test_client)
{
	AOption *opt = NULL;
	AOptionDecode(&opt, "async_tcp:{address:127.0.0.1,port:4444}", -1);
	opt->refcount = 1;

	int count = 5000;
	TRACE("input test client num(default 5000):\r\n");
	fscanf(stdin, "%d", &count);
	char buf[412];
	fgets(buf, sizeof(buf), stdin);
	fgets(buf, sizeof(buf), stdin);

	for (int ix = 0; ix < count; ++ix) {
		client_t *c = gomake(client_t);
		AObject::create(&c->io, NULL, opt, NULL);

		opt->addref();
		c->msg.init(opt);
		c->msg.done = &c_msg_done;
		c->asop.timer();
		c->asop.done = &c_asop_done;
		c->data[0] = '\0';

		int result = c->io->open(&c->msg);
		if (result < 0)
			c->msg.done2(result);
	}
	opt->delref();
}

static int on_event(const char *name, bool preproc, void *p)
{
	AClientComponent *c = (AClientComponent*)p;
	TRACE("%s: user = %s(%p, %p), preproc = %d.\n",
		name, c->_object->_module->module_name, c->_object, c, preproc);

	MqttComponent *mqtt; c->_other(&mqtt);
	if (mqtt != NULL) {
		TRACE("this is a mqtt event...\n");
	}
	return 1;
}
#if 1
CU_TEST(test_pvd)
{
	//return;
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

	AEntity *e = NULL;
	result = AObject::create(&e, NULL, opt, NULL);
	opt->release();
	AClientComponent *c; e->_get(&c);

	ASystemManager *sm = ASystemManager::get();
	AEventManager *em = sm->_event_manager;
	em->lock();
	em->_sub_self(em, "on_client_opened", c, &on_event)->_oneshot = true;
	em->_sub_self(em, "on_client_opened", c, &on_event)->_oneshot = false;
	em->_sub_self(em, "on_client_closed", c, &on_event)->_oneshot = true;
	em->_sub_self(em, "on_client_closed", c, &on_event)->_oneshot = false;
	em->unlock();

	AEntityManager *etm = sm->_all_entities;
	etm->lock();
	etm->_push(etm, e);
	etm->unlock();

	sm->lock();
	sm->_regist(e); e->release();
	sm->unlock();
}
CU_TEST(test_mqtt)
{
	AOption *opt = NULL;
	AOptionDecode(&opt, "MQTTClient: { io: io_openssl { "
		"io: async_tcp { address: test.mosquitto.org, port: 8883, },"
		"}, }", -1);

	AEntity *mqtt = NULL;
	int result = AObject::create(&mqtt, NULL, opt, NULL);
	opt->release();

	AClientComponent *c; mqtt->_get(&c);
	ASystemManager *sm = ASystemManager::get();
	AEventManager *em = sm->_event_manager;
	em->lock();
	em->_sub_self(em, "on_client_opened", c, &on_event)->_oneshot = true;
	em->_sub_self(em, "on_client_opened", c, &on_event)->_oneshot = false;
	em->_sub_self(em, "on_client_closed", c, &on_event)->_oneshot = true;
	em->_sub_self(em, "on_client_closed", c, &on_event)->_oneshot = false;
	em->unlock();

	AEntityManager *etm = sm->_all_entities;
	etm->lock();
	etm->_push(etm, mqtt);
	etm->unlock();

	sm->lock();
	sm->_regist(mqtt); mqtt->release();
	sm->unlock();
}
#endif
#endif

int main()
{
	dlload(NULL, "io_openssl", FALSE);
	dlload(NULL, "service_http", FALSE);
	dlload(NULL, "mqtt_client", FALSE);
	AModuleInit(NULL);
	AThreadBegin(NULL, NULL, 1000);

	ASystemManager *sm = ASystemManager::get();
	sm->start_checkall(sm);

	CuString *output = CuStringNew();
	CuSuiteRun(all_test_suites);
	CuSuiteSummary(all_test_suites, output);
	CuSuiteDetails(all_test_suites, output);
	fputs(output->buffer, stdout);
	CuStringDelete(output);
	CuSuiteDelete(all_test_suites);

	TRACE("press enter for end..............................\n");
	getchar();

	reset_nif(sm->_all_services, NULL, {
		TRACE("AServiceStop():..............................\n");
		AServiceStop(sm->_all_services, TRUE);
		sm->_all_services->release();
	});
	sm->stop_checkall(sm);
	sm->clear_allsys(true);
	sm->_event_manager->clear_sub();
	sm->_all_entities->_clear(sm->_all_entities);

	getchar();
	AThreadEnd(NULL);
	AModuleExit();
#if defined(_WIN32) && defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
