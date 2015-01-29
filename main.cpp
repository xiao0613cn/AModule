// InterfaceMethod.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "base/AModule.h"
#include "base/async_operator.h"
#include "PVDClient/PvdNetCmd.h"


static const char *pvd_path =
	"PVDClient: PVDClient {"
	"       io: tcp {"
	"		address: 192.168.10.21,"
	"		port: 8000,"
	"               timeout: 5,"
	"	},"
	"	username: ':18',"
	"	password: 888888,"
	"	channel: 0,"
	"	linkmode: 0,"
	"}";

struct RecvMsg {
	AMessage msg;
	async_operator op;
	AObject *pvd;
};

long CloseDone(AMessage *msg, long result)
{
	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	TRACE("%p: close result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, msg->type&~AMsgType_Custom, msg->size);

	AObjectRelease(rm->pvd);
	free(rm);
	return result;
}

static BOOL g_abort = FALSE;

long RecvDone(AMessage *msg, long result)
{
	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	if (result >= 0) {
		//TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		//	rm->pvd, result, msg->type&~AMsgType_Custom, msg->size);
		// for next recv
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		if (g_abort)
			result = rm->pvd->cancel(rm->pvd, ARequest_MsgLoop|ARequest_Output, NULL);
	} else {
		CloseDone(msg, result);
	}
	return result;
}

DWORD WINAPI RecvCB2(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = &RecvDone;
	INIT_LIST_HEAD(&rm->msg.entry);

	//long result = rm->pvd->request(rm->pvd, ANotify_InQueueFront|ARequest_Output, &rm->msg);
	long result = rm->pvd->request(rm->pvd, ARequest_MsgLoop|ARequest_Output, &rm->msg);
	if (result != 0) {
		TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
			rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		rm->msg.done = CloseDone;
		CloseDone(&rm->msg, result);
	}
	return result;
}
void RecvCB(async_operator *op, int result)
{
	RecvMsg *rm = CONTAINING_RECORD(op, RecvMsg, op);
	if (result >= 0) {
		RecvCB2(rm);
	} else {
		CloseDone(&rm->msg, result);
	}
}
DWORD WINAPI SendHeart(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	INIT_LIST_HEAD(&rm->msg.entry);

	pvdnet_head header;
	rm->msg.type = AMsgType_Custom|NET_SDVR_SHAKEHAND;
	rm->msg.data = (char*)&header;
	rm->msg.size = PVDCmdEncode(0, &header, NET_SDVR_SHAKEHAND, 0);
	rm->msg.done = NULL;

	long result;
	do {
		::Sleep(3000);
		result = rm->pvd->request(rm->pvd, ARequest_Input, &rm->msg);
	} while (!g_abort && (result > 0));
	TRACE("%p: send result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

	result = rm->pvd->cancel(rm->pvd, ARequest_MsgLoop|ARequest_Output, NULL);
	result = rm->pvd->request(rm->pvd, ARequest_Input, &rm->msg);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;
	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0) {
		CloseDone(&rm->msg, result);
	}
	return result;
}

DWORD WINAPI RecvStream(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	rm->msg.done = NULL;
	INIT_LIST_HEAD(&rm->msg.entry);

	long result = 0;
	do {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, 0, &rm->msg);
		//TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		//	rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);
	} while (!g_abort && (result > 0));
	TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;
	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0) {
		CloseDone(&rm->msg, result);
	}
	return result;
}

void ResetOption(AOption *option)
{
	TRACE("%s[%s] = ", option->name, option->value);

	char value[MAX_PATH];
	value[0] = '\0';
	if (gets_s(value) == NULL)
		return;
	if (value[0] != '\0')
		strcpy_s(option->value, value);

	AOption *child;
	list_for_each_entry(child, &option->children_list, AOption, brother_entry) {
		ResetOption(child);
	}
}

extern AModule SyncControlModule;
extern AModule TCPModule;
extern AModule PVDClientModule;
extern AModule PVDRTModule;
extern AModule TCPServerModule;
extern AModule HTTPProxyModule;

void test_pvd(AOption *option)
{
	AObject *pvd = NULL;
	AObject *rt = NULL;
	RecvMsg *rm;
	AMessage sm;
	long result;
_retry:
	ResetOption(option);
	AMsgInit(&sm, AMsgType_Option, (char*)option, sizeof(AOption));
	sm.done = NULL;

	if (_stricmp(option->value, "PVDClient") == 0) {
		release_s(pvd, AObjectRelease, NULL);
		result = SyncControlModule.create(&pvd, NULL, option);
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
		QueueUserWorkItem(&RecvCB2, rm, 0);
		rm = NULL;

		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; AObjectAddRef(pvd);
		QueueUserWorkItem(&SendHeart, rm, 0);
		rm = NULL;
	}
	if (pvd != NULL) {
		strcpy_s(option->value, "PVDRTStream");
		result = SyncControlModule.create(&rt, pvd, option);
		//result = PVDRTModule.create(&rt, pvd, option);
	}
	if (result >= 0) {
		result = rt->open(rt, &sm);
		TRACE("%p: open result = %d.\n", rt, result);
	}
	if (result > 0) {
		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = rt; AObjectAddRef(rt);
		QueueUserWorkItem(&RecvStream, rm, 0);
		rm = NULL;
	}

	release_s(rt, AObjectRelease, NULL);

	//async_thread_end(&at);
	TRACE("input 'r' to retry...\n");
	char str[256];
	while (gets_s(str) != NULL) {
		if (str[0] == '\0')
			continue;
		if (str[0] == 'r') {
			goto _retry;
		}
		if (strcmp(str, "quit") == 0)
			break;
	}
	release_s(pvd, AObjectRelease, NULL);
	g_abort = TRUE;
	::Sleep(3000);
}

static const char *proxy_path =
	"tcp_server: tcp_server {"
	"	port: 8080,"
	"	io: tcp,"
	"	HTTPProxy {"
	"		io: tcp {"
	"			address: 127.0.0.1,"
	"			port: 80,"
	"		},"
	"	},"
	"	default_bridge {"
	"		address: 127.0.0.1,"
	"		port: 8000,"
	"	},"
	"},";

void test_proxy(AOption *option)
{
	AMessage msg;

	AObject *tcp_server = NULL;
	long result = AObjectCreate(&tcp_server, NULL, option, NULL);
	if (result >= 0) {
		AMsgInit(&msg, AMsgType_Option, (char*)option, sizeof(*option));
		msg.done = NULL;
		result = tcp_server->open(tcp_server, &msg);
	}
	TRACE("proxy(%s) open = %d.\n", option->name, result);
	char str[256];
	gets_s(str);
	if (tcp_server != NULL)
		tcp_server->close(tcp_server, NULL);
	release_s(tcp_server, AObjectRelease, NULL);
}

int _tmain(int argc, _TCHAR* argv[])
{
	AModuleRegister(&TCPModule);
	AModuleRegister(&PVDClientModule);
	AModuleRegister(&SyncControlModule);
	AModuleRegister(&PVDRTModule);
	AModuleRegister(&TCPServerModule);
	//AModuleRegister(&HTTPProxyModule);
	AModuleInitAll(NULL);

	//extern int async_test(void);
	//async_test();
	//async_thread at;
	//memset(&at, 0, sizeof(at));
	//async_thread_begin(&at, NULL);

	char str[256];
	TRACE("input test module(pvd, ...)\n");
	gets_s(str);

	const char *path;
	if (_strnicmp(str, "pvd", 3) == 0)
		path = pvd_path;
	else
		path = proxy_path;

	AOption *option = NULL;
	long result = AOptionDecode(&option, path);
	if (result == 0)
	{
		if (path == pvd_path)
			test_pvd(option);
		else
			test_proxy(option);
	}
	gets_s(str);
	release_s(option, AOptionRelease, NULL);
	_CrtDumpMemoryLeaks();
	return 0;
}

