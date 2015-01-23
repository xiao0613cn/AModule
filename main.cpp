// InterfaceMethod.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "base/AModule.h"
#include "base/async_operator.h"
#include "PVDClient/PvdNetCmd.h"


#define REGISTER_MODULE(name) \
	extern AModule name; \
	AModuleRegister(&name);

static const char *addr_path =
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

int _tmain(int argc, _TCHAR* argv[])
{
	REGISTER_MODULE(TCPModule);
	REGISTER_MODULE(PVDClientModule);
	REGISTER_MODULE(SyncControlModule);
	REGISTER_MODULE(PVDRTModule);
	AModuleInitAll();

	//extern int async_test(void);
	//async_test();
	//async_thread at;
	//memset(&at, 0, sizeof(at));
	//async_thread_begin(&at, NULL);

	long result;
	AOption *option = NULL;
	result = AOptionDecode(&option, addr_path);
	if (result != 0) {
		release_s(option, AOptionRelease, NULL);
		return result;
	}

	AObject *pvd = NULL;
	AObject *rt = NULL;
	RecvMsg *rm;
	AMessage sm;
_retry:
	ResetOption(option);
	AMsgInit(&sm, AMsgType_Option, (char*)option, sizeof(AOption));
	sm.done = NULL;

	if (_stricmp(option->value, PVDClientModule.module_name) == 0) {
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
		strcpy_s(option->value, PVDRTModule.module_name);
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
	gets_s(str);

	release_s(option, AOptionRelease, NULL);
	_CrtDumpMemoryLeaks();
	return 0;
}

