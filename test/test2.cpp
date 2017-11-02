#include "stdafx.h"
#include "base/AModule_API.h"
#ifdef _WIN32
#pragma comment(lib, "..\\bin\\AModule.lib")
#endif
#include "base/spinlock.h"
#include "ecs/AEntity.h"
#include "ecs/AEvent.h"
#include "ecs/ASystem.h"
#include "ecs/AInOutComponent.h"
#include "test.h"


static int on_event(void *user, const char *name, void *p, bool preproc) {
	TRACE("user = %p, name = %s, p = %p, preproc = %d.\n", user, name, p, preproc);
	return 1;
}

ASystemManager sm;

CuSuite all_test_suites;

#if defined TEST_ECHO_SERVICE
CU_TEST(test_echo_service)
{
	AOption *opt = NULL;
	AOptionDecode(&opt, "tcp_server: { port: 4444, io: async_tcp, "
		"is_async: 1, services: { EchoService, HttpService, }, background: 0 }", -1);

	AService *tcp_server = NULL;
	AObject::create(&tcp_server, NULL, opt, NULL);
	tcp_server->sysmng = &sm;

	tcp_server->start(tcp_server, opt);
	opt->release();
}
#elif defined TEST_ECHO_CLIENT
struct echo_info {
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
	const int thread_num = 10;
	echo_info ei[thread_num];
	pthread_t tid[thread_num];
	for (int ix = 0; ix < thread_num; ++ix) {
		pthread_create(&tid[ix], NULL, &echo_thread, &ei[ix]);
	}

	int total_qps = 0;
	uint64_t total_speed = 0;
	for (int ix = 0; ix < thread_num; ++ix)
	{
		pthread_join(tid[ix], NULL);
		TRACE("%2d: count = %d, diff = %.3f, recv_size = %lld KB.\n",
			ix, ei[ix].count, ei[ix].duration/1000.0, ei[ix].recv_size/1024);

		int qps = ei[ix].count*1000/ei[ix].duration; total_qps += qps;
		uint64_t speed = ei[ix].recv_size*1000/ei[ix].duration/1024; total_speed += speed;
		TRACE("%2d: qps = %d, speed = %lld KBps.\n", ix, qps, speed);
	}
	TRACE("total_qps = %d, total_speed = %lld KBps.\n", total_qps, total_speed);
}
#else
void* test_entity_run(void*)
{
	for (;;) {
		sm.check_allsys(&sm, GetTickCount());
		::Sleep(10);
	}
	return NULL;
}
CU_TEST(test_entity)
{
	return;
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

	AEntity *mqtt = NULL;
	AOptionDecode(&opt, "MQTTClient: { io: io_openssl { "
		"io: async_tcp { address: test.mosquitto.org, port: 8883, },"
		"}, }", -1);
	result = AObject::create(&mqtt, NULL, opt, NULL);
	opt->release();

	sm._event_manager->_sub_const("on_client_opened", false, e, &on_event);
	sm._event_manager->_sub_const("on_client_opened", true, e, &on_event);
	sm._event_manager->_sub_const("on_client_opened", true, mqtt, &on_event);
	sm._event_manager->_sub_const("on_client_closed", true, e, &on_event);
	sm._event_manager->_sub_const("on_client_closed", true, mqtt, &on_event);
	sm._event_manager->_sub_const("on_client_closed", false, e, &on_event);

	sm._regist(e);
	sm._regist(mqtt);

	//sm._unregist(e); e->release();
	//sm._unregist(mqtt); mqtt->release();
	pthread_post(NULL, &test_entity_run);
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
	if (c->data[0] == '\0') {
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
	AOptionDecode(&opt, "async_tcp:{address:192.168.40.17,port:4444}", -1);

	for (int ix = 0; ix < 5000; ++ix) {
		client_t *c = gomake(client_t);
		AObject::create(&c->io, NULL, opt, NULL);
		
		c->msg.init(opt);
		c->msg.done = &c_msg_done;
		c->asop.timer();
		c->asop.done = &c_asop_done;
		c->data[0] = '\0';

		int result = c->io->open(&c->msg);
		if (result < 0)
			c->msg.done2(result);
	}
}
#endif

int main()
{
	dlload(NULL, "io_openssl", FALSE);
	AModuleInit(NULL);
	AThreadBegin(NULL, NULL, 1000);

	AEventManager em; em.init();
	sm.init();
	sm._event_manager = &em;

	CuString *output = CuStringNew();
	CuSuiteRun(&all_test_suites);
	CuSuiteSummary(&all_test_suites, output);
	CuSuiteDetails(&all_test_suites, output);
	fputs(output->buffer, stdout);

#if defined(_WIN32) && defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif
	getchar();
	return 0;
}
