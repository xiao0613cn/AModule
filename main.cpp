// InterfaceMethod.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <process.h>
#include "base/AModule_API.h"
#include "io/AModule_io.h"
#include "PVDClient/PvdNetCmd.h"

#ifndef _WINDLL

static const char *pvd_path =
	"stream: PVDClient {"
	"       io: async_tcp {"
	"		address: 127.0.0.1,"
	"		port: 8000,"
	"               timeout: 5,"
	"	},"
	"	username: ':40',"
	"	password: 888888,"
	"	force_alarm: 0,"
	"	channel: 0,"
	"	linkmode: 0,"
	"	channel_count: ,"
	"	m3u8_proxy: 0,"
	"}";

struct RecvMsg {
	AMessage msg;
	AOperator op;
	AObject *pvd;
	long     reqix;
};

long CloseDone(AMessage *msg, long result)
{
	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	TRACE("%p: close result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, msg->type&~AMsgType_Custom, msg->size);

	aobject_release(rm->pvd);
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
		amsg_init(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, rm->reqix, &rm->msg);
	} while (!g_abort && (result > 0));

	TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

	amsg_init(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = CloseDone;

	result = rm->pvd->close(rm->pvd, &rm->msg);
	if (result != 0)
		CloseDone(&rm->msg, result);
	return result;
}
void RecvCB(AOperator *op, int result)
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
		result = rm->pvd->request(rm->pvd, Aio_Input, &rm->msg);
	} while (!g_abort && (result > 0));

	TRACE("%p: send result = %d, msg type = %d, size = %d.\n",
		rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);

	//result = rm->pvd->cancel(rm->pvd, ARequest_MsgLoop|Aio_Output, NULL);
	result = rm->pvd->request(rm->pvd, Aio_Input, &rm->msg);

	amsg_init(&rm->msg, AMsgType_Unknown, NULL, 0);
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

	if (value[0] == ' ')
		option->value[0] = '\0';
	else if (value[0] != '\0')
		strcpy_s(option->value, value);

	AOption *child;
	list_for_each_entry(child, &option->children_list, AOption, brother_entry) {
		ResetOption(child);
	}
}

extern AModule PVDClientModule;
extern AModule PVDRTModule;

void test_pvd(AOption *option, bool reset_option)
{
	AObject *pvd = NULL;
	AObject *rt = NULL;
	RecvMsg *rm;
	AMessage sm;
	long result;
_retry:
	if (reset_option)
		ResetOption(option);
	amsg_init(&sm, AMsgType_Option, (char*)option, 0);
	sm.done = NULL;

	if (_stricmp(option->value, "PVDClient") == 0) {
		release_s(pvd, aobject_release, NULL);
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
		rm->pvd = pvd; aobject_addref(pvd);
		rm->reqix = Aio_Output;
		CloseHandle((HANDLE)_beginthreadex(NULL, 0, &RecvCB2, rm, 0, NULL));
		rm = NULL;

		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = pvd; aobject_addref(pvd);
		rm->reqix = Aio_Input;
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
		rm->pvd = rt; aobject_addref(rt);
		rm->reqix = 0;
		CloseHandle((HANDLE)_beginthreadex(NULL, 0, &RecvCB2, rm, 0, NULL));
		rm = NULL;
	}
	release_s(rt, aobject_release, NULL);

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
	release_s(pvd, aobject_release, NULL);
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
	long result = aobject_create(&tcp_server, NULL, option, NULL);
	if (result >= 0) {
		amsg_init(&msg, AMsgType_Option, (char*)option, 0);
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
	release_s(tcp_server, aobject_release, NULL);
}

int main(int argc, char* argv[])
{
	AOption *option = NULL;
	long result;
	if (argc > 1) {
		result = aoption_decode(&option, argv[1]);
		if (result < 0)
			release_s(option, aoption_release, NULL);
	}
	if (option == NULL) {
		result = aoption_decode(&option, pvd_path);
		if (option != NULL)
			ResetOption(option);
	}
	amodule_init_option(option);
	option = NULL;

	athread_begin(NULL, NULL);
	extern AModule TCPModule; amodule_register(&TCPModule);
	extern AModule TCPServerModule; amodule_register(&TCPServerModule);
	extern AModule AsyncTcpModule; amodule_register(&AsyncTcpModule);
	extern AModule SyncControlModule; amodule_register(&SyncControlModule);
	extern AModule PVDClientModule; amodule_register(&PVDClientModule);
	extern AModule PVDRTModule; amodule_register(&PVDRTModule);
	extern AModule HTTPProxyModule; amodule_register(&HTTPProxyModule);
	extern AModule PVDProxyModule; amodule_register(&PVDProxyModule);
	extern AModule DumpModule; amodule_register(&DumpModule);
	extern AModule M3U8ProxyModule; amodule_register(&M3U8ProxyModule);
	extern AModule EchoModule; amodule_register(&EchoModule);

	char str[256];
	const char *path;
	if (argc > 2) {
		result = aoption_decode(&option, argv[2]);
		if (result < 0)
			release_s(option, aoption_release, NULL);
	}
	bool reset_option = false;
	if (option == NULL) {
		reset_option = true;
		do {
			TRACE("input test module: pvd, tcp_server ...\n");
			gets_s(str);
			if (_stricmp(str,"pvd") == 0)
				path = pvd_path;
			else if (/*str[0] == '\0' || */_stricmp(str,"tcp_server") == 0)
				path = proxy_path;
			else if (str[0] == 'q')
				goto _return;
			else
				continue;
			break;
		} while (1);
		result = aoption_decode(&option, path);
	}
	if (result == 0) {
		if (_stricmp(option->name, "stream") == 0)
			test_pvd(option, reset_option);
		else if (_stricmp(option->name, "server") == 0)
			test_proxy(option, reset_option);
		else
			TRACE("unknown test module [%s: %s]...\n", option->name, option->value);
	}
	release_s(option, aoption_release, NULL);
_return:
	amodule_exit();
	gets_s(str);
	athread_end(NULL);
#ifdef _DEBUG
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