// InterfaceMethod.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <process.h>
#include "base/AModule.h"
#include "base/async_operator.h"
#include "PVDClient/PvdNetCmd.h"


static const char *pvd_path =
	"PVDClient: PVDClient {"
	"       io: tcp {"
	"		address: 192.168.20.174,"
	"		port: 8101,"
	"               timeout: 5,"
	"	},"
	"	username: 'admin',"
	"	password: 888888,"
	"	channel_count: ,"
	"	alarm_in_count: ,"
	"	alarm_out_count: ,"
	"	hdd_count: ,"
	"	channel: 0,"
	"	linkmode: 0,"
	"}";

struct RecvMsg {
	AMessage msg;
	async_operator op;
	AObject *pvd;
	long     reqix;
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

unsigned int WINAPI RecvCB2(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	rm->msg.done = NULL;

	long result;
	do {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, rm->reqix, &rm->msg);
	} while (!g_abort && (result > 0));

	TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;

	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0)
		CloseDone(&rm->msg, result);
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
unsigned int WINAPI SendHeart(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;

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

	//result = rm->pvd->cancel(rm->pvd, ARequest_MsgLoop|ARequest_Output, NULL);
	result = rm->pvd->request(rm->pvd, ARequest_Input, &rm->msg);

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
extern AModule AsyncTcpModule;
extern AModule PVDClientModule;
extern AModule PVDRTModule;
extern AModule TCPServerModule;
extern AModule HTTPProxyModule;
extern AModule PVDProxyModule;

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
		rm->reqix = ARequest_Output;
		CloseHandle((HANDLE)_beginthreadex(NULL, 0, &RecvCB2, rm, 0, NULL));
		rm = NULL;

		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; AObjectAddRef(pvd);
		rm->reqix = ARequest_Input;
		CloseHandle((HANDLE)_beginthreadex(NULL, 0, &SendHeart, rm, 0, NULL));
		rm = NULL;
	}
	if (pvd != NULL) {
		strcpy_s(option->value, "PVDRTStream");
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
		CloseHandle((HANDLE)_beginthreadex(NULL, 0, &RecvCB2, rm, 0, NULL));
		rm = NULL;
	}
	release_s(rt, AObjectRelease, NULL);

	//async_thread_end(&at);
	char str[256];
	do {
		TRACE("input 'r' for retry, input 'q' for quit...\n");
		gets_s(str);
		if (str[0] == 'r')
			goto _retry;
		if (str[0] == 'q')
			break;
	} while (1);
	release_s(pvd, AObjectRelease, NULL);
	g_abort = TRUE;
	::Sleep(3000);
}

static const char *proxy_path =
	"tcp_server: tcp_server {"
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

void test_proxy(AOption *option)
{
	AMessage msg;
	ResetOption(option);

	AObject *tcp_server = NULL;
	long result = AObjectCreate(&tcp_server, NULL, option, NULL);
	if (result >= 0) {
		AMsgInit(&msg, AMsgType_Option, (char*)option, sizeof(*option));
		msg.done = NULL;
		result = tcp_server->open(tcp_server, &msg);
	}

	TRACE("proxy(%s) open = %d.\n", option->name, result);
	char str[256];
	do {
		TRACE("input 'q' for quit...\n");
		gets_s(str);
		if (str[0] == 'q')
			break;
	} while (1);

	if (tcp_server != NULL)
		tcp_server->close(tcp_server, NULL);
	release_s(tcp_server, AObjectRelease, NULL);
}

int _tmain(int argc, _TCHAR* argv[])
{
	async_thread_begin(NULL, NULL);
	AModuleRegister(&TCPModule);
	AModuleRegister(&TCPServerModule);
	AModuleRegister(&AsyncTcpModule);
	AModuleRegister(&SyncControlModule);
	AModuleRegister(&PVDClientModule);
	AModuleRegister(&PVDRTModule);
	AModuleRegister(&HTTPProxyModule);
	AModuleRegister(&PVDProxyModule);

	AOption *option = NULL;
	long result = AOptionDecode(&option, pvd_path);
	ResetOption(option);
	AModuleInitAll(option);
	release_s(option, AOptionRelease, NULL);

	char str[256];
	const char *path;
	do {
		TRACE("input test module: pvd, tcp_server ...\n");
		gets_s(str);
		if (_stricmp(str,"pvd") == 0)
			path = pvd_path;
		else if (_stricmp(str,"tcp_server") == 0)
			path = proxy_path;
		else if (str[0] == 'q')
			goto _return;
		else
			continue;
		break;
	} while (1);
	result = AOptionDecode(&option, path);
	if (result == 0)
	{
		if (path == pvd_path)
			test_pvd(option);
		else
			test_proxy(option);
	}
	release_s(option, AOptionRelease, NULL);
_return:
	AModuleExitAll();
	gets_s(str);
	async_thread_end(NULL);
#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

