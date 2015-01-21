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
	"		address: 192.168.20.37,"
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
	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	if (result >= 0) {
		// match test, for quit notify queue.
		TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
			rm->pvd, result, msg->type&~AMsgType_Custom, msg->size);
		return -1;
	}
	if (rm->msg.size != 0) {
		// quit notify queue, process this message
		// ...
		// ...
		// re-register notify queue
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, ANotify_InQueueFront|ARequest_Output, &rm->msg);
	} else {
		TRACE("%p: closed or release, result = %d.\n", rm->pvd, result);
	}
	if (result < 0) {
		AObjectRelease(rm->pvd);
		free(rm);
	}
	return result;
}

DWORD WINAPI RecvCB2(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
	rm->msg.done = &RecvDone;
	INIT_LIST_HEAD(&rm->msg.entry);

	long result = rm->pvd->request(rm->pvd, ANotify_InQueueFront|ARequest_Output, &rm->msg);
	rm->pvd->request(rm->pvd, ARequest_MsgLoop|ARequest_Output, NULL);
	if (result < 0) {
		AObjectRelease(rm->pvd);
		free(rm);
	}
	return result;
}
void RecvCB(async_operator *op, int result)
{
	RecvMsg *rm = CONTAINING_RECORD(op, RecvMsg, op);
	if (result >= 0) {
		RecvCB2(rm);
	} else {
		AObjectRelease(rm->pvd);
		free(rm);
	}
}

long StreamDone(AMessage *msg, long result)
{
	RecvMsg *rm = CONTAINING_RECORD(msg, RecvMsg, msg);
	//TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
	//	rm->pvd, result, msg->type&~AMsgType_Custom, msg->size);
	if (result >= 0) {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, ARequest_Input, &rm->msg);
	}
	if (result < 0) {
		TRACE("%p: closed or release, result = %d.\n", rm->pvd, result);
		AObjectRelease(rm->pvd);
		free(rm);
	}
	return result;
}

DWORD WINAPI RecvStream(void *p)
{
	RecvMsg *rm = (RecvMsg*)p;
	rm->msg.done = &StreamDone;
	INIT_LIST_HEAD(&rm->msg.entry);

	long result = 0;
	do {
		AMsgInit(&rm->msg, AMsgType_Unknown, NULL, 0);
		result = rm->pvd->request(rm->pvd, ARequest_Input, &rm->msg);
		//TRACE("%p: recv result = %d, msg type = %d, size = %d.\n",
		//	rm->pvd, result, rm->msg.type&~AMsgType_Custom, rm->msg.size);
	} while (result > 0);
	if (result < 0) {
		TRACE("%p: closed or release, result = %d.\n", rm->pvd, result);
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
	REGISTER_MODULE(PVDRTModule);
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
	TRACE("%p: open result = %d.\n", pvd, result);
	if (result < 0) {
		release_s(pvd, AObjectRelease, NULL);
		release_s(option, AOptionRelease, NULL);
		return result;
	}

	//extern int async_test(void);
	//async_test();
	//async_thread at;
	//memset(&at, 0, sizeof(at));
	//async_thread_begin(&at, NULL);

	RecvMsg *rm = (RecvMsg*)malloc(sizeof(RecvMsg));
	rm->pvd = pvd; AObjectAddRef(pvd);
	//rm->op.callback = &RecvCB;
	//async_operator_post(&rm->op, &at, 0);
	QueueUserWorkItem(&RecvCB2, rm, 0);
	rm = NULL;

	AObject *rt = NULL;
	strcpy_s(option->value, PVDRTModule.module_name);
	result = SyncControlModule.create(&rt, pvd, option);
	if (result >= 0) {
		result = rt->open(rt, &sm);
		TRACE("%p: open result = %d.\n", rt, result);
	}
	if (result > 0) {
		rm = (RecvMsg*)malloc(sizeof(RecvMsg));
		rm->pvd = rt; AObjectAddRef(rt);
		//rm->op.callback = &RecvCB;
		//async_operator_post(&rm->op, &at, 0);
		QueueUserWorkItem(&RecvStream, rm, 0);
		rm = NULL;
	} else {
		release_s(rt, AObjectRelease, NULL);
	}

	pvdnet_head header;
	sm.type = AMsgType_Custom|NET_SDVR_SHAKEHAND;
	sm.data = (char*)&header;
	sm.size = PVDCmdEncode(0, &header, NET_SDVR_SHAKEHAND, 0);
	sm.done = NULL;
	for (int ix = 0; ix < 10000000; ++ix) {
		::Sleep(3000);
		result = pvd->request(pvd, ARequest_Input, &sm);
	}
	if (rt != NULL) {
		rt->close(rt, NULL);
		AObjectRelease(rt);
		rt = NULL;
	}
	//::Sleep(300);
	result = pvd->cancel(pvd, ARequest_Output, NULL);
	TRACE("cancel output = %d.\n", result);

	//::Sleep(300);
	// wake up the Output MsgLoop
	result = pvd->request(pvd, ARequest_Input, &sm);

	::Sleep(300);
	sm.done = CloseDone;
	result = pvd->close(pvd, &sm);
	TRACE("do close = %d.\n", result);
	if (result != 0)
		CloseDone(&sm, result);

	release_s(pvd, AObjectRelease, NULL);
	release_s(option, AOptionRelease, NULL);
	//result = pvd->close(pvd, &sm);

	//async_thread_end(&at);
	::Sleep(1000);

	_CrtDumpMemoryLeaks();
	return 0;
}

