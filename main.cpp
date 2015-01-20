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
	"PVDClient {"
	"       io: tcp {"
	"		address: 192.168.20.228,"
	"		port: 8101,"
	"               timeout: 5,"
	"	},"
	"	rt: tcp,"
	"	his: stream {"
	"		async_tcp,"
	"	main: udp  ,"
	"}}";

struct RecvMsg {
	AMessage msg;
	async_operator op;
	AObject *pvd;
};

long CloseDone(AMessage *msg, long result)
{
	TRACE("close result = %d, msg type = %d, size = %d.\n",
		result, msg->type&~AMsgType_Custom, msg->size);
	/*RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);

	AObjectRelease(rm->pvd);
	free(rm);*/
	return result;
}

long RecvDone(AMessage *msg, long result)
{
	TRACE("recv result = %d, msg type = %d, size = %d.\n",
		result, msg->type&~AMsgType_Custom, msg->size);

	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	if (result >= 0) {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		return result;
	}

	/*rm->msg.done = CloseDone;
	result = rm->pvd->close(rm->pvd, &rm->msg);
	TRACE("do close = %d.\n", result);*/
	//if (result != 0) {
		AObjectRelease(rm->pvd);
		free(rm);
	//}
	return result;
}

void RecvCB(async_operator *op, int result)
{
	RecvMsg *rm = CONTAINING_RECORD(op, RecvMsg, op);

	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = &RecvDone;
	result = rm->pvd->request(rm->pvd, ARequest_MsgLoop|ARequest_Output, &rm->msg);
	if (result < 0) {
		AObjectRelease(rm->pvd);
		free(rm);
	}
}
DWORD WINAPI RecvCB2(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = &RecvDone;

	long result = rm->pvd->request(rm->pvd, ARequest_MsgLoop|ARequest_Output, &rm->msg);
	if (result < 0) {
		AObjectRelease(rm->pvd);
		free(rm);
	}
	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	REGISTER_MODULE(TCPModule);
	REGISTER_MODULE(PVDClientModule);
	REGISTER_MODULE(SyncControlModule);
	AModuleInitAll();

	long result;
	AOption *option = NULL;
	result = AOptionDecode(&option, addr_path);
	if (result != 0) {
		release_s(option, AOptionRelease, NULL);
		return result;
	}

	AMessage sm;
	AMsgInit(&sm, AMsgType_Option, (char*)option, sizeof(AOption));
	sm.done = NULL;

	AObject *pvd = NULL;
	result = SyncControlModule.create(&pvd, NULL, option);
	if (result >= 0)
		result = pvd->open(pvd, &sm);
	TRACE("open result = %d.\n", result);
	/*if (result < 0) {
		release_s(pvd, AObjectRelease, NULL);
		release_s(option, AOptionRelease, NULL);
		return result;
	}*/

	//extern int async_test(void);
	//async_test();
	async_thread at;
	memset(&at, 0, sizeof(at));
	async_thread_begin(&at, NULL);

	RecvMsg *rm = (RecvMsg*)malloc(sizeof(RecvMsg));
	rm->pvd = pvd; AObjectAddRef(pvd);
	rm->op.callback = &RecvCB;
	async_operator_post(&rm->op, &at, 0);
	//QueueUserWorkItem(&RecvCB, rm, 0);

	pvdnet_head header;
	sm.type = AMsgType_Custom|NET_SDVR_SHAKEHAND;
	sm.data = (char*)&header;
	sm.size = PVDCmdEncode(0, &header, NET_SDVR_SHAKEHAND, 0);
	sm.done = NULL;
	for (int ix = 0; ix < 20; ++ix) {
		::Sleep(300);
		result = pvd->request(pvd, ARequest_Input, &sm);
	}

	//::Sleep(3000);
	result = pvd->cancel(pvd, ARequest_Output, &rm->msg);
	TRACE("cancel output = %d.\n", result);

	//::Sleep(3000);
	/*wake up the Output MsgLoop*/
	result = pvd->request(pvd, ARequest_Input, &sm);

	//::Sleep(3000);
	sm.done = CloseDone;
	result = pvd->close(pvd, &sm);
	TRACE("do close = %d.\n", result);
	if (result != 0)
		CloseDone(&sm, result);

	release_s(pvd, AObjectRelease, NULL);
	release_s(option, AOptionRelease, NULL);
	//result = pvd->close(pvd, &sm);

	async_thread_end(&at);
	//::Sleep(10000);

	_CrtDumpMemoryLeaks();
	return 0;
}

