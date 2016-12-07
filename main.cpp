#include "stdafx.h"
//#include <map>
#include "base/AModule_API.h"
#include "io/AModule_io.h"
#include "PVDClient/PvdNetCmd.h"
extern "C" {
#include "http/http_parser.h"
};

DWORD async_test_tick;
AOperator asop_list[20];

void async_test_callback(AOperator *asop, int result)
{
	TRACE("asop->timeout = %d, diff = %d, result = %d.\n",
		int(asop-asop_list), GetTickCount()-async_test_tick, result);
}

int async_test(void)
{
	AThread *at;
	int result = AThreadBegin(&at, NULL, 20*1000);
	TRACE("AThreadBegin(%p) = %d...\n", at, result);

	async_test_tick = GetTickCount();
	int diff = 100;

	for (int ix = 0; ix < _countof(asop_list); ++ix) {
		asop_list[ix].callback = async_test_callback;
		AOperatorPost(&asop_list[ix], at, async_test_tick+ix/2*diff*2);
	}

	Sleep(diff);
	AOperatorSignal(&asop_list[5], at, 0);
	Sleep(12*diff);
	AOperatorSignal(&asop_list[18], at, 0);

	char buf[BUFSIZ];
	fgets(buf, sizeof(buf), stdin);
	AThreadEnd(at);
#if defined(_DEBUG) && defined(_WIN32)
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

struct myrb_node {
	struct rb_node  rb_node;
	int    key;
};
static inline int myrb_cmp(int key, myrb_node *data)
{
	if (key < data->key)
		return -1;
	if (key > data->key)
		return 1;
	return 0;
}
rb_tree_define(myrb_node, rb_node, int, myrb_cmp)

void rbtree_test()
{
	/*std::map<int, int> test_set;
	test_set.insert(std::make_pair(5, 5));
	test_set.insert(std::make_pair(9, 9));
	test_set.insert(std::make_pair(14, 14));
	test_set.insert(std::make_pair(18, 18));

	std::map<int, int>::iterator it = test_set.upper_bound(3);
	it = test_set.upper_bound(8);
	it = test_set.upper_bound(14);
	it = test_set.upper_bound(24);
	it = test_set.lower_bound(14);*/

	srand(rand());
	struct rb_root root;
	INIT_RB_ROOT(&root);

	myrb_node *r;
	int test_count = 20000;
	int insert_count = 0;
	int search_count = test_count*1000;

	for (int ix = 0; ix < test_count; ++ix) {
		r = (myrb_node*)malloc(sizeof(myrb_node));
		r->key = rand();

		if (rb_insert_myrb_node(&root, r, r->key) != NULL)
			free(r);
		else
			++insert_count;
	}
	TRACE("rbtree insert = %d.\n", insert_count);

	DWORD tick = GetTickCount();
	for (int ix = 0; ix < search_count; ++ix) {
		r = rb_search_myrb_node(&root, ix);
	}
	TRACE("rbtree search %d times, tick = %d.\n", search_count, GetTickCount()-tick);

	//
	/*std::map<int, myrb_node> map;
	myrb_node r2;

	insert_count = 0;
	for (int ix = 0; ix < test_count; ++ix) {
		r2.key = rand();
		if (map.insert(std::make_pair(r2.key, r2)).second)
			++insert_count;
	}
	TRACE("map<int> insert = %d, size = %d.\n", insert_count, map.size());

	tick = GetTickCount();
	for (int ix = 0; ix < search_count; ++ix) {
		map.find(ix);
	}
	TRACE("map<int> search %d times, tick = %d.\n", search_count, GetTickCount()-tick);*/

	while (!RB_EMPTY_ROOT(&root)) {
		r = rb_entry(rb_first(&root), myrb_node, rb_node);

		rb_erase(&r->rb_node, &root);
		free(r);
	}
	getchar();
}

#if 1
extern AModule TCPModule;
extern AModule TCPServerModule;
extern AModule AsyncTcpModule;
extern AModule SyncControlModule;
extern AModule PVDClientModule;
extern AModule PVDRTModule;
extern AModule HTTPProxyModule;
extern AModule PVDProxyModule;
extern AModule DumpModule;
#if 0
extern AModule M3U8ProxyModule;
#endif
extern AModule EchoModule;


#ifndef _WINDLL

static const char *pvd_path =
	"stream: PVDClient {"
	"       io: async_tcp {"
	"		address: '192.168.20.163',"
	"		port: 8101,"
	"               timeout: 5,"
	"	},"
	"	username: 'admin',"
	"	password: '888888',"
	"	force_alarm: 0,"
	"	channel: 0,"
	"	linkmode: 0,"
	"	channel_count: ,"
	"	m3u8_proxy: 0,"
	"	proactive: 1 {"
	"		io: async_tcp {"
	"			address: '192.168.20.16',"
	"			port: 8000,"
	"			timeout: 5,"
	"		},"
	"		prefix: ,"
	"		first: 100,"
	"	},"
	"}";

struct RecvMsg {
	AMessage msg;
	AOperator op;
	AObject *pvd;
	int     reqix;
};

int CloseDone(AMessage *msg, int result)
{
	RecvMsg *rm = container_of(msg, RecvMsg, msg);
	TRACE("%p: close result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, msg->type&~AMsgType_Private, msg->size);

	AObjectRelease(rm->pvd);
	free(rm);
	return result;
}

static BOOL g_abort = FALSE;

void* RecvCB2(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	rm->msg.done = NULL;

	int result;
	do {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, rm->reqix, &rm->msg);
	} while (!g_abort && (result > 0));

	TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Private, rm->msg.size);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;

	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0)
		CloseDone(&rm->msg, result);
	return NULL;
}
int RecvCB(AOperator *op, int result)
{
	RecvMsg *rm = container_of(op, RecvMsg, op);
	if (result >= 0) {
		RecvCB2(rm);
	} else {
		CloseDone(&rm->msg, result);
	}
	return 0;
}
void* SendHeart(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;

	pvdnet_head header;
	rm->msg.type = AMsgType_Private|NET_SDVR_SHAKEHAND;
	rm->msg.data = (char*)&header;
	rm->msg.size = PVDCmdEncode(0, &header, NET_SDVR_SHAKEHAND, 0);
	rm->msg.done = NULL;

	int result;
	do {
		::Sleep(3000);
		result = ioInput(rm->pvd, &rm->msg);
	} while (!g_abort && (result > 0));

	TRACE("%p: send result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Private, rm->msg.size);

	//result = rm->pvd->cancel(rm->pvd, ARequest_MsgLoop|Aio_Output, NULL);
	result = ioInput(rm->pvd, &rm->msg);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;
	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0) {
		CloseDone(&rm->msg, result);
	}
	return NULL;
}

void ResetOption(AOption *option)
{
	TRACE("%s[%s] = ", option->name, option->value);

	char value[BUFSIZ];
	value[0] = '\0';
	if (fgets(value, sizeof(value), stdin) == NULL)
		return;

	size_t sep = strcspn(value, " \r\n");
	value[sep] = '\0';

	if (value[0] == ' ')
		option->value[0] = '\0';
	else if (value[0] != '\0')
		strcpy_sz(option->value, value);

	AOption *child;
	list_for_each_entry(child, &option->children_list, AOption, brother_entry) {
		ResetOption(child);
	}
}

void test_pvd(AOption *option, bool reset_option)
{
	AObject *pvd = NULL;
	AObject *rt = NULL;
	RecvMsg *rm;
	AMessage sm;
	int result;
	pthread_t thread;
_retry:
	if (reset_option)
		ResetOption(option);
	AMsgInit(&sm, AMsgType_Option, option, 0);
	sm.done = NULL;

	if (_stricmp(option->value, "PVDClient") == 0) {
		release_s(pvd, AObjectRelease, NULL);
		//result = SyncControlModule.create(&pvd, NULL, option);
		result = PVDClientModule.create(&pvd, NULL, option);
	} else {
		result = -1;
	}
	if (result >= 0) {
		result = pvd->open(pvd, &sm);
		TRACE("%p: open result = %d.\n", pvd, result);
	}
	if (result > 0) {
		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; AObjectAddRef(pvd);
		rm->reqix = Aio_Output;
		pthread_create(&thread, NULL, &RecvCB2, rm);
		pthread_detach(thread);
		rm = NULL;

		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; AObjectAddRef(pvd);
		rm->reqix = Aio_Input;
		pthread_create(&thread, NULL, &SendHeart, rm);
		pthread_detach(thread);
		rm = NULL;
	}
	if (pvd != NULL) {
		strcpy_sz(option->value, "PVDRTStream");
		//result = SyncControlModule.create(&rt, pvd, option);
		result = PVDRTModule.create(&rt, pvd, option);
	}
	if (result >= 0) {
		result = rt->open(rt, &sm);
		TRACE("%p: open result = %d.\n", rt, result);
	}
	if (result > 0) {
		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = rt; AObjectAddRef(rt);
		rm->reqix = 0;
		pthread_create(&thread, NULL, &RecvCB2, rm);
		pthread_detach(thread);
		rm = NULL;
	}
	release_s(rt, AObjectRelease, NULL);

	//async_thread_end(&at);
	char str[256];
	for ( ; ; ) {
		TRACE("input 'r' for retry, input 'q' for quit...\n");
		fgets(str, sizeof(str), stdin);
		if (str[0] == 'r')
			goto _retry;
		if (str[0] == 'q')
			break;
	}
	release_s(pvd, AObjectRelease, NULL);
	g_abort = TRUE;
	::Sleep(3000);
}

static const char *proxy_path =
	"server: tcp_server {"
	"	port: 8101,"
	"	io: async_tcp,"
	"	HTTPProxy: bridge {"
	"		address: www.sina.com.cn,"
	"		port: 80,"
	"	},"
	"	PVDProxy {"
	"		address: 192.168.10.21, "
	"		port: 8101, "
	"	},"
	"	default_bridge {"
	"		address: 127.0.0.1,"
	"		port: 8000,"
	"	},"
	"},";

void test_proxy(AOption *option, bool reset_option)
{
	AMessage msg;
	if (reset_option)
		ResetOption(option);

	AObject *tcp_server = NULL;
	int result = AObjectCreate(&tcp_server, NULL, option, NULL);
	if (result >= 0) {
		AMsgInit(&msg, AMsgType_Option, option, 0);
		msg.done = NULL;
		result = tcp_server->open(tcp_server, &msg);
	}

	TRACE("proxy(%s) open = %d.\n", option->name, result);
	char str[256];
	for ( ; ; ) {
		TRACE("input 'q' for quit...\n");
		fgets(str, sizeof(str), stdin);
		if (str[0] == 'q')
			break;
	}

	if (tcp_server != NULL)
		tcp_server->close(tcp_server, NULL);
	release_s(tcp_server, AObjectRelease, NULL);
}

static const char *proactive_path =
	"proactive: 1 {"
	"	create: 1,"
	"	first_delay: 5000,"
	"	duration: 1000,"
	"},";

void proactive_wait(AOperator *asop, int result)
{
	RecvMsg *rm = container_of(asop, RecvMsg, op);

	rm->reqix = 1;
	AMsgInit(&rm->msg, AMsgType_Option, NULL, 0);

	result = rm->pvd->open(rm->pvd, &rm->msg);
	if (result != 0)
		result = rm->msg.done(&rm->msg, result);
}

int proactive_done(AMessage *msg, int result)
{
	RecvMsg *rm = container_of(msg, RecvMsg, msg);

	if (rm->reqix == 4) {
		TRACE("proactive close done = %d.\n", result);
		AOperatorTimewait(&rm->op, NULL, 15*1000);
		return result;
	}

	if (rm->reqix == 1) {
		TRACE("proactive open done = %d.\n", result);
		if (result == 0)
			result = 1;
	}

	while (result > 0) {
		if (rm->reqix != 2) {
			rm->reqix = 2;
			AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		} else {
			rm->reqix = 3;
		}

		result = ioInput(rm->pvd, &rm->msg);
		if (result <= 0)
			break;

		if (rm->reqix == 2)
			result = ioOutput(rm->pvd, &rm->msg);
	}

	if (result < 0) {
		TRACE("proactive request error = %d.\n", result);
		rm->reqix = 4;
		result = rm->pvd->close(rm->pvd, &rm->msg);
		if (result != 0)
			rm->msg.done(&rm->msg, result);
	}
	return result;
}

void test_proactive(AOption *option, bool reset_option)
{
	char str[BUFSIZ];
	for ( ; ; ) {
		if (reset_option)
			ResetOption(option);

		int delay = AOptionChildInt(option, "first_delay");
		int duration = AOptionChildInt(option, "duration");

		int create = AOptionChildInt(option, "create");
		for (int ix = 0; ix < create; ++ix) {
			AObject *p = NULL;
			int result = AObjectCreate(&p, NULL, NULL, "PVDProxy");
			if (result < 0)
				break;

			RecvMsg *rm = (RecvMsg*)malloc(sizeof(RecvMsg));
			rm->msg.done = &proactive_done;
			rm->op.callback = &proactive_wait;
			rm->pvd = p;

			result = AOperatorTimewait(&rm->op, NULL, delay+ix*duration);
		}
		AOptionChild2(&option->children_list, "create", str)[0] = '\0';
		AOptionChild2(&option->children_list, "first_delay", str)[0] = '\0';

		TRACE("input 'q' for quit...\n");
		fgets(str, sizeof(str), stdin);
		if (str[0] == 'q')
			break;
	}
}

int http_cb_name(http_parser *parser, const char *name)
{
	TRACE("%s...\n", name);
	return 0;
}
int http_data_cb_name(http_parser *parser, const char *at, size_t len, const char *name)
{
	AMessage *msg = (AMessage*)parser->data;
	strncpy(msg->data+msg->size, at, len);
	msg->size += len;

	msg->data[msg->size] = '\0';
	TRACE("%s: %s, len = %d.\n", name, msg->data, msg->size);
	msg->size = 0;
	return 0;
}

#define http_cb_test(name)  \
int http_cb_##name(http_parser *parser) { \
	return http_cb_name(parser, #name); \
}
#define http_data_cb_test(name) \
int http_data_cb_##name(http_parser *parser, const char *at, size_t len) { \
	return http_data_cb_name(parser, at, len, #name); \
}

http_cb_test(message_begin)
http_data_cb_test(url)
http_data_cb_test(status)
http_data_cb_test(header_field)
http_data_cb_test(header_value)
http_cb_test(headers_complete)
http_data_cb_test(body)
http_cb_test(message_complete)
http_cb_test(chunk_header)
http_cb_test(chunk_complete)

#define http_parser_settings_cb_test(settings, name) \
	settings.on_##name = &http_cb_##name;
#define http_parser_settings_data_cb_test(settings, name) \
	settings.on_##name = &http_data_cb_##name;

void http_parser_test()
{
#ifdef _WIN32
	char buf[BUFSIZ];
	AMessage msg;
	AMsgInit(&msg, 0, buf, 0);

	struct http_parser parser;
	http_parser_init(&parser, HTTP_BOTH, &msg);
	parser.flags |= F_UPGRADE;

	struct http_parser_settings settings;
	http_parser_settings_init(&settings);
	http_parser_settings_cb_test(settings, message_begin);
	http_parser_settings_data_cb_test(settings, url);
	http_parser_settings_data_cb_test(settings, status);
	http_parser_settings_data_cb_test(settings, header_field);
	http_parser_settings_data_cb_test(settings, header_value);
	http_parser_settings_cb_test(settings, headers_complete);
	http_parser_settings_data_cb_test(settings, body);
	http_parser_settings_cb_test(settings, message_complete);
	http_parser_settings_cb_test(settings, chunk_header);
	http_parser_settings_cb_test(settings, chunk_complete);

	const char *str = "GET /test/b HTTP/1.1\r\n"
         "User-Agent: curl/7.18.0 (i486-pc-linux-gnu) libcurl/7.18.0 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.1\r\n"
         "Host: 0.0.0.0=5000\r\n"
         "Accept: */*\r\n"
	 "Transfer-Encoding: chunked\r\n"
	 "\r\n"
	 "8\r\nbody=xxx\r\n"
	 "2\r\nab\r\n"
	 "0\r\n\r\n"
	 "next_protocol\r\n";

	size_t len = strlen(str);
	int ix = 0;
	while (ix < len) {
		size_t ret = http_parser_execute(&parser, &settings, str+ix, 5);

		TRACE("http_parser_execute(%d) = %d, http_errno = %s.\n",
			5, ret, http_parser_error(&parser));
		if (parser.http_errno != HPE_OK)
			break;
		ix += ret;

		if (http_next_chunk_is_incoming(&parser)) {
			fprintf(stdout, "http msg len = %d, done: %d, left = %s\n", len, ix, str+ix);
			break;
		}
		if (http_header_is_complete(&parser)) {
			fprintf(stdout, "http header done, left = %s\n", str+ix);
		}
	}
	fgets(buf, BUFSIZ, stdin);
#endif
}

int test_run(AOption *option, bool reset_option)
{
	AOption *opt = AOptionFind(option, "object");
	AMessage msg = { 0 };

	AObject *object = NULL;
	int ret = AObjectCreate(&object, NULL, NULL, opt?opt->value:NULL);
	if (ret >= 0) {
		opt = AOptionFind(option, "open");
		AMsgInit(&msg, AMsgType_Option, opt, 0);

		ret = object->open(object, &msg);
		TRACE("open(%s) = %d.\n", opt?opt->value:"", ret);
	}
	if (ret >= 0) {
		int reqix = AOptionChildInt(option, "reqix", 0);

		const char *str = AOptionChild(option, "request", "");
		AMsgInit(&msg, AMsgType_Unknown, str, strlen(str));

		ret = object->request(object, reqix, &msg);
		TRACE("request(%d, %s) = %d.\n", reqix, str, ret);
	}
	if (object != NULL) {
		ret = object->close(object, &msg);
		TRACE("close() = %d.\n", ret);

		AObjectRelease(object);
	}
	return ret;
}

int main(int argc, char* argv[])
{
	TRACE("sizeof(int) = %d, sizeof(long) = %d, sizeof(void*) = %d, sizeof(long long) = %d.\n",
		sizeof(int), sizeof(long), sizeof(void*), sizeof(long long));
	//async_test();
	//rbtree_test();
	http_parser_test();

	AOption *option = NULL;
	int result;
	if (argc > 1) {
		result = AOptionDecode(&option, argv[1]);
	} else if (option == NULL) {
		result = AOptionDecode(&option, pvd_path);
		if (option != NULL)
			ResetOption(option);
	}
	AModuleInitOption(option);
	option = NULL;

	AThreadBegin(NULL, NULL, 20*1000);
	AModuleRegister(&TCPModule);
	AModuleRegister(&TCPServerModule);
	AModuleRegister(&AsyncTcpModule);
	AModuleRegister(&SyncControlModule);
	AModuleRegister(&PVDClientModule);
	AModuleRegister(&PVDRTModule);
	AModuleRegister(&HTTPProxyModule);
	AModuleRegister(&PVDProxyModule);
	AModuleRegister(&DumpModule);
#if 0
	AModuleRegister(&M3U8ProxyModule);
#endif
	AModuleRegister(&EchoModule);

	char str[256];
	const char *path;
	if (argc > 2) {
		result = AOptionDecode(&option, argv[2]);
	}
	bool reset_option = false;
	if (option == NULL) {
		reset_option = true;
		for ( ; ; ) {
			TRACE("input test module: pvd, tcp_server, proactive ...\n");
			fgets(str, sizeof(str), stdin);
			if (_stricmp(str, "pvd") == 0)
				path = pvd_path;
			else if (/*str[0] == '\0' || */_stricmp(str, "tcp_server") == 0)
				path = proxy_path;
			else if (_stricmp(str, "proactive") == 0)
				path = proactive_path;
			else if (str[0] == 'q')
				goto _return;
			else
				continue;
			break;
		}
		result = AOptionDecode(&option, path);
	}
	if (result == 0) {
		if (_stricmp(option->name, "stream") == 0) {
			test_pvd(option, reset_option);
		} else if (_stricmp(option->name, "server") == 0) {
			test_proxy(option, reset_option);
		} else if (_stricmp(option->name, "proactive") == 0) {
			test_proactive(option, reset_option);
		} else if (_stricmp(option->name, "test_run") == 0) {
			test_run(option, reset_option);
		} else {
			TRACE("unknown test module [%s: %s]...\n", option->name, option->value);
		}
	}
	release_s(option, AOptionRelease, NULL);
_return:
	fgets(str, sizeof(str), stdin);
	AModuleExit();
	fgets(str, sizeof(str), stdin);
	AThreadEnd(NULL);
#if defined(_DEBUG) && defined(_WIN32)
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

#else

BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, void *pReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif
#endif
