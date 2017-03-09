#include "stdafx.h"
//#include <map>
#include "base/AModule_API.h"

extern AModule TCPModule;
extern AModule FileModule;
extern AModule TCPServerModule;
extern AModule AsyncTcpModule;
extern AModule SyncControlModule;
extern AModule PVDClientModule;
extern AModule PVDRTModule;
extern AModule HTTPProxyModule;
extern AModule PVDProxyModule;
extern AModule DumpModule;
extern AModule EchoModule;
extern AModule HttpClientModule;

#ifndef _WINDLL
#ifdef _WIN32
extern AModule M3U8ProxyModule;
#endif
#include "io/AModule_io.h"
#include "PVDClient/PvdNetCmd.h"
extern "C" {
#include "http/http_parser.h"
#include "base/wait_queue.h"
};
#include "rpc/AModule_rpc.h"

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
rb_tree_declare(myrb_node, int)
rb_tree_define(myrb_node, rb_node, int, myrb_cmp)

void __stdcall rbtree_test()
{
#ifdef _MAP_
	std::map<int, int> test_set;
	test_set.insert(std::make_pair(5, 5));
	test_set.insert(std::make_pair(9, 9));
	test_set.insert(std::make_pair(14, 14));
	test_set.insert(std::make_pair(18, 18));

	std::map<int, int>::iterator it;
	it = test_set.find(3);
	it = test_set.upper_bound(3);
	it = test_set.lower_bound(3);
	it = test_set.find(8);
	it = test_set.upper_bound(8);
	it = test_set.lower_bound(8);
	it = test_set.find(12);
	it = test_set.upper_bound(12);
	it = test_set.lower_bound(12);
	it = test_set.find(14);
	it = test_set.upper_bound(14);
	it = test_set.lower_bound(14);
	it = test_set.find(24);
	it = test_set.upper_bound(24);
	it = test_set.lower_bound(24);
#endif

	srand(rand());
	struct rb_root root;
	INIT_RB_ROOT(&root);

	myrb_node *r;
	int test_count = 20000;
	int insert_count = 0;
	int search_count = 32767;

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
		r = rb_find_myrb_node(&root, ix);
	}
	TRACE("rbtree search %d times, tick = %d.\n", search_count, GetTickCount()-tick);
	r = rb_find_myrb_node(&root, 500);
	r = rb_upper_myrb_node(&root, 500);
	r = rb_lower_myrb_node(&root, 500);
	r = rb_find_myrb_node(&root, 600);
	r = rb_upper_myrb_node(&root, 600);
	r = rb_lower_myrb_node(&root, 600);
	r = rb_find_myrb_node(&root, 700);
	r = rb_upper_myrb_node(&root, 700);
	r = rb_lower_myrb_node(&root, 700);
	r = rb_find_myrb_node(&root, 700000);
	r = rb_upper_myrb_node(&root, 700000);
	r = rb_lower_myrb_node(&root, 700000);

#ifdef _MAP_
	std::map<int, myrb_node> map;
	myrb_node r2;
	srand(rand());

	//insert_count = 0;
	for (int ix = 0; ix < insert_count; /*++ix*/) {
		r2.key = rand();
		if (map.insert(std::make_pair(r2.key, r2)).second)
			++ix;
	}
	TRACE("map<int> insert = %d, size = %d.\n", insert_count, map.size());

	tick = GetTickCount();
	for (int ix = 0; ix < search_count; ++ix) {
		map.find(ix);
	}
	TRACE("map<int> search %d times, tick = %d.\n", search_count, GetTickCount()-tick);

	tick = GetTickCount();
	for (int ix = 0; ix < search_count; ++ix) {
		r = rb_find_myrb_node(&root, ix);
	}
	TRACE("rbtree search %d times, tick = %d.\n", search_count, GetTickCount()-tick);
#endif

	while (!RB_EMPTY_ROOT(&root)) {
		r = rb_entry(rb_first(&root), myrb_node, rb_node);

		rb_erase(&r->rb_node, &root);
		free(r);
	}
	getchar();
}

static const char *pvd_path =
	"stream: PVDClient {"
	"       io: tcp {"
	"		address: '192.168.60.227',"
	"		port: 8101,"
	"               timeout: 5,"
	"	},"
	"	username: 'admin',"
	"	password: '888888',"
	"	channel: 0,"
	"	linkmode: 0,"
	"}";

static const char *g_opt = 
"global_param {"
"PVDProxy {"
"       io: async_tcp {"
"		address: '192.168.60.227',"
"		port: 8101,"
"               timeout: 5,"
"	},"
"	username: 'admin',"
"	password: '888888',"
"	force_alarm: 0,"
"	channel: 0,"
"	linkmode: 0,"
"	channel_count: ,"
"	proactive: 1 {"
"		io: async_tcp {"
"			address: '192.168.20.16',"
"			port: 8000,"
"			timeout: 5,"
"		},"
"		prefix: ,"
"		first: 100,"
"	},"
"},"
"M3U8Proxy: 0,"
"}";

struct RecvMsg {
	AMessage msg;
	AOperator op;
	AObject *pvd;
	int     reqix;
	AOption *option;
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
		if (rm->reqix == Aio_Output) {
		for (int ix = 0; ix < 10; ++ix) {
			Sleep(3000);
			pvdnet_head header;
			rm->msg.type = AMsgType_Private|NET_SDVR_SHAKEHAND;
			rm->msg.data = (char*)&header;
			rm->msg.size = PVDCmdEncode(0, &header, NET_SDVR_SHAKEHAND, 0);
			ioInput(rm->pvd, &rm->msg);

			rm->msg.init();
			ioOutput(rm->pvd, &rm->msg);
		} }

		rm->msg.init();
		result = rm->pvd->request(rm->pvd, rm->reqix, &rm->msg);
		Sleep(5000);

		result = rm->pvd->close(rm->pvd, NULL);
		result = rm->pvd->close(rm->pvd, &rm->msg);
		TRACE("%p: close() = %d.\n", rm->pvd, result);

		rm->msg.init(rm->option);
		result = rm->pvd->open(rm->pvd, &rm->msg);
		TRACE("%p: open() = %d.\n", rm->pvd, result);
	} while (!g_abort);

	TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Private, rm->msg.size);

	rm->msg.init();
	rm->msg.done = CloseDone;

	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0)
		CloseDone(&rm->msg, result);
	return NULL;
}
void RecvCB(AOperator *op, int result)
{
	RecvMsg *rm = container_of(op, RecvMsg, op);
	if (result >= 0) {
		RecvCB2(rm);
	} else {
		CloseDone(&rm->msg, result);
	}
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
		result = AObjectCreate2(&pvd, NULL, option, &PVDClientModule);
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
		rm->option = AOptionClone(option, NULL);
		rm->op.callback = &RecvCB;
		pthread_create(&thread, NULL, &RecvCB2, rm);
		pthread_detach(thread);
		rm = NULL;

		/*rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; AObjectAddRef(pvd);
		rm->reqix = Aio_Input;
		pthread_create(&thread, NULL, &SendHeart, rm);
		pthread_detach(thread);
		rm = NULL;*/
	}
	if (pvd != NULL) {
		strcpy_sz(option->value, "PVDRTStream");
		result = AObjectCreate2(&rt, pvd, option, &PVDRTModule);
	}
	if (result >= 0) {
		result = rt->open(rt, &sm);
		TRACE("%p: open result = %d.\n", rt, result);
	}
	if (result > 0) {
		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = rt; AObjectAddRef(rt);
		rm->reqix = 0;
		rm->option = AOptionClone(option, NULL);
		rm->op.callback = &RecvCB;
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
	"	port: 8080,"
	"	io: async_tcp,"
	"	HTTPProxy {"
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

		int delay = AOptionGetInt(option, "first_delay");
		int duration = AOptionGetInt(option, "duration");

		int create = AOptionGetInt(option, "create");
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
		AOptionGet2(&option->children_list, "create", str)[0] = '\0';
		AOptionGet2(&option->children_list, "first_delay", str)[0] = '\0';

		TRACE("input 'q' for quit...\n");
		fgets(str, sizeof(str), stdin);
		if (str[0] == 'q')
			break;
	}
}

int http_cb_name(http_parser *parser, const char *name)
{
	TRACE("%s...\n", name);
	if ((_stricmp(name, "chunk_complete") == 0)
	 || (_stricmp(name, "message_complete") == 0))
		http_parser_pause(parser, TRUE);
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
		if (parser.http_errno == HPE_PAUSED)
			http_parser_pause(&parser, FALSE);

		if (parser.http_errno != HPE_OK) {
			TRACE("http_parser_execute(%d) = %d, str = %s, http_errno = %s.\n",
				5, ret, str+ix, http_parser_error(&parser));
			break;
		}
		ix += ret;

		ret = http_next_chunk_is_incoming(&parser);
		if (ret != 0) {
			fprintf(stdout, "http msg len = %d, done = %d, left = %s\n", len, ret, str+ix);
			if (ret != 2)
				break;
		} else if (http_header_is_complete(&parser)) {
			fprintf(stdout, "http header done, left = %s\n", str+ix);
		}
	}
	fgets(buf, BUFSIZ, stdin);
}
/*
{ object: http_client, open { io: io_dump { file_name: test_run, io : tcp { address: www.sina.com.cn, port : 80, } }, method : GET, ':Host' : www.sina.com.cn, version: HTTP/1.1, }, request { reqix: 0 }, request { reqix: 1 }, request { reqix: 0 }, request { reqix: 1 }, request { reqix: 0 }, request { reqix: 1 }, request { reqix: 0 }, request { reqix: 1 }, }
*/
int test_run(AOption *option, bool reset_option)
{
	if (option != NULL) {
		if (reset_option)
			ResetOption(option);
		reset_option = false;
	} else {
		char buf[BUFSIZ];
		do {
			fgets(buf, BUFSIZ, stdin);
		} while ((buf[0] == '\r') || (buf[0] == '\n'));

		int result = AOptionDecode(&option, buf);
		if (result < 0) {
			release_s(option, AOptionRelease, NULL);
			return result;
		}
		reset_option = true;
	}
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
		AOption *child;
		list_for_each_entry(child, &option->children_list, AOption, brother_entry)
		{
			if (_stricmp(child->name, "request") != 0)
				continue;

			int reqix = AOptionGetInt(child, "reqix", 0);

			const char *str = AOptionGet(child, "data", "");
			AMsgInit(&msg, AMsgType_Unknown, str, strlen(str));

			ret = object->request(object, reqix, &msg);
			TRACE("request(%d, %s) = %d, data = %s.\n", reqix, str, ret, msg.data);
		}
	}
	if (object != NULL) {
		AMsgInit(&msg, AMsgType_Unknown, NULL, 0);
		ret = object->close(object, &msg);
		TRACE("close() = %d.\n", ret);

		AObjectRelease(object);
	}
	if (reset_option)
		AOptionRelease(option);
	return ret;
}

void __stdcall a1(int a, long long d)
{
	a;// b; c;
}

struct at {
	int a;
	short b;
	short c;
	int d;
};
int a2(at a)
{
	a.a; a.b; a.c; a.d;
	return a.d;
}

template <int size>
struct param_traits {
	int  argv[size];
	param_traits(void *data) {
		memcpy(argv, data, sizeof(argv));
	}
};

int main(int argc, char* argv[])
{
	/*char buf[12] = { 1, 0, 0, 0, 2, 0, 3, 0, 4, 0, 0, 0 };
	void (__stdcall*p1)(param_traits<3>) = (void (__stdcall*)(param_traits<3>))&a1;
	p1(param_traits<3>(buf));

	rpc_void_argv<rpc_argv2<arg_traits<int>, arg_traits<long long> > > rpc_argv;
	rpc_argv.init();
	rpc_argv.param._a1.data = 1;
	rpc_argv.param._a2.data = 0x00040302;
	p1(param_traits<3>(&rpc_argv.param));*/

	TRACE("sizeof(int) = %d, sizeof(long) = %d, sizeof(void*) = %d, sizeof(long long) = %d.\n",
		sizeof(int), sizeof(long), sizeof(void*), sizeof(long long));
	//async_test();
	//rbtree_test();
	//http_parser_test();

	AOption *option = NULL;
	int result;
	if (argc > 1) {
		result = AOptionDecode(&option, argv[1]);
	} else if (option == NULL) {
		result = AOptionDecode(&option, g_opt);
		if (option != NULL)
			ResetOption(option);
	}
	AModuleInitOption(option);
	option = NULL;

	AThreadBegin(NULL, NULL, 20*1000);
	AModuleRegister(&TCPModule);
	AModuleRegister(&FileModule);
	AModuleRegister(&TCPServerModule);
	AModuleRegister(&AsyncTcpModule);
	AModuleRegister(&SyncControlModule);
	AModuleRegister(&PVDClientModule);
	AModuleRegister(&PVDRTModule);
	AModuleRegister(&HTTPProxyModule);
	AModuleRegister(&PVDProxyModule);
	AModuleRegister(&DumpModule);
#ifdef _WIN32
	AModuleRegister(&M3U8ProxyModule);
#endif
	AModuleRegister(&EchoModule);
	AModuleRegister(&HttpClientModule);

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
			if (_strnicmp_c(str, "pvd") == 0)
				path = pvd_path;
			else if (/*str[0] == '\0' || */_strnicmp_c(str, "tcp_server") == 0)
				path = proxy_path;
			else if (_strnicmp_c(str, "proactive") == 0)
				path = proactive_path;
			else if (_strnicmp_c(str, "test_run") == 0) {
				test_run(NULL, reset_option);
				continue;
			}
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
		AModuleInitOption(NULL);
		AThreadBegin(NULL, NULL, 20*1000);
		AModuleRegister(&TCPModule);
		AModuleRegister(&FileModule);
		AModuleRegister(&TCPServerModule);
		AModuleRegister(&AsyncTcpModule);
		AModuleRegister(&SyncControlModule);
		AModuleRegister(&PVDClientModule);
		AModuleRegister(&PVDRTModule);
		AModuleRegister(&HTTPProxyModule);
		AModuleRegister(&PVDProxyModule);
		AModuleRegister(&DumpModule);
	//	AModuleRegister(&M3U8ProxyModule);
		AModuleRegister(&EchoModule);
		AModuleRegister(&HttpClientModule);
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
