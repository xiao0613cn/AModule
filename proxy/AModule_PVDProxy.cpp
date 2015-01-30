#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "../PVDClient/PvdNetCmd.h"

static AObject *pvd = NULL;
static AObject *rt = NULL;

struct PVDProxy {
	void       *extend;
	void      (*release)(AObject*);
	long      (*open)(AObject*,AMessage*);

	AObject    *client;
	AObject    *object;
	AMessage    outmsg;
	SliceBuffer outbuf;
};

static void PVDProxyRelease(AObject *object)
{
	PVDProxy *p = (PVDProxy*)object->extend;
	assert(p->client == object);

	object->extend = p->extend;
	object->release = p->release;
	object->open = p->open;

	release_s(p->client, p->release, NULL);
	release_s(p->object, AObjectRelease, NULL);
	SliceFree(&p->outbuf);
	free(p);
}

static long PVDProxyTryOutput(PVDProxy *p)
{
}

static long PVDProxyOpen(AObject *object, AMessage *msg)
{
	PVDProxy *p = (PVDProxy*)object->extend;
	assert(p->client == object);

	long result = SliceResize(&p->outbuf, max(8*1024,msg->size));
	if (result < 0)
		return -ENOMEM;

	p->outmsg.type = msg->type;
	p->outmsg.size = msg->size;
	memcpy(SliceResPtr(&p->outbuf), msg->data, msg->size);
	do {
		SlicePush(&p->outbuf, p->outbuf.size);
		result = PVDProxyTryOutput(p);
		if (result < 0)
			break;
		if (result == 0) {
			AMsgInit(&p->outmsg, AMsgType_Unknown, SliceResPtr(&p->outbuf), SliceResLen(&p->outbuf));
			result = p->client->request(p->client, ARequest_Output, &p->outmsg);
			continue;
		}

		AMsgInit(&p->outmsg, AMsgType_Custom|p->outmsg.type, NULL, 0);
		result = p->pvd->request(p->pvd, ANotify_InQueueBack|ARequest_Output, &p->outmsg);
		if (result <= 0)
			break;

		memcpy(SliceResPtr(&p->outbuf), p->outmsg.data, 
			result = p->pvd->request(p->pvd, ARequest_Input, &p->outmsg);
	} while (result > 0);
	return result;
}

static long PVDProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	if ((pvd == NULL) || (parent == NULL)) {
		return -EFAULT;
	}

	PVDProxy *p = (PVDProxy*)malloc(sizeof(PVDProxy));
	if (p == NULL)
		return -ENOMEM;

	p->client = parent;
	p->object = pvd; AObjectAddRef(pvd);
	SliceInit(&p->outbuf);

	p->extend = parent->extend; parent->extend = p;
	p->release = parent->release; parent->release = &PVDProxyRelease;
	p->open = parent->open; parent->open = &PVDProxyOpen;
	AObjectAddRef(parent);
	*object = parent;
	return 1;
}

//////////////////////////////////////////////////////////////////////////
struct HeartMsg {
	AMessage msg;
	AObject *object;
	union {
	pvdnet_head heart;
	struct {
	long        reqix;
	};
	};
};
static void HeartMsgFree(HeartMsg *sm, long result)
{
	TRACE("result = %d.\n", result);
	release_s(sm->object, AObjectRelease, NULL);
	free(sm);
}
static long PVDSendDone(AMessage *msg, long result);
static DWORD WINAPI PVDSendProc(void *p)
{
	HeartMsg *sm = (HeartMsg*)p;
	long result;
	do {
		::Sleep(3000);
		sm->msg.type = AMsgType_Custom|NET_SDVR_SHAKEHAND;
		sm->msg.data = (char*)&sm->heart;
		sm->msg.size = PVDCmdEncode(0, &sm->heart, NET_SDVR_SHAKEHAND, 0);
		sm->msg.done = &PVDSendDone;
		result = sm->object->request(sm->object, ARequest_Input, &sm->msg);
	} while (result > 0);
	if (result != 0) {
		HeartMsgFree(sm, result);
	}
	return result;
}
static long PVDSendDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result > 0) {
		QueueUserWorkItem(&PVDSendProc, sm, 0);
	} else {
		HeartMsgFree(sm, result);
	}
	return result;
}
static long PVDRecvDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result > 0) {
		/*pvdnet_head *phead = (pvdnet_head*)msg->data;
		MSHEAD *mshead = (MSHEAD*)msg->data;
		STREAM_HEADER *shead = (STREAM_HEADER*)msg->data;
		if ((phead->uFlag == NET_CMD_HEAD_FLAG)
		 || (ISMSHEAD(mshead) && ISKEYFRAME(mshead))
		 || (shead->nHeaderFlag == STREAM_HEADER_FLAG && shead->nFrameType == STREAM_FRAME_VIDEO_I))
			TRACE("result = %d.\n", result);*/
		AMsgInit(&sm->msg, AMsgType_Unknown, NULL, 0);
	} else {
		HeartMsgFree(sm, result);
	}
	return result;
}
static DWORD WINAPI PVDRecvProc(void *p)
{
	HeartMsg *sm = (HeartMsg*)p;
	AMsgInit(&sm->msg, AMsgType_Unknown, NULL, 0);
	sm->msg.done = &PVDRecvDone;

	long result = sm->object->request(sm->object, ARequest_MsgLoop|sm->reqix, &sm->msg);
	if (result != 0)
		HeartMsgFree(sm, result);
	return result;
}

long PVDProxyInit(AOption *option)
{
	long result = -EFAULT;
	AOption opt;
	AOptionInit(&opt, NULL);

	AModule *syncControl = AModuleFind("stream", "SyncControl");
	if (syncControl != NULL) {
		strcpy_s(opt.name, "PVDClient");
		result = syncControl->create(&pvd, NULL, &opt);
	}
	HeartMsg *sm = NULL;
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; AObjectAddRef(pvd);

		AMsgInit(&sm->msg, AMsgType_Option, (char*)option, sizeof(*option));
		sm->msg.done = NULL;
		result = sm->object->open(sm->object, &sm->msg);

		TRACE("open(%s) result = %d.\n", opt.name, result);
		if (result < 0) {
			HeartMsgFree(sm, result);
		} else {
			QueueUserWorkItem(&PVDSendProc, sm, 0);
		}
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; AObjectAddRef(pvd);
		sm->reqix = ARequest_Output;
		QueueUserWorkItem(&PVDRecvProc, sm, 0);

		strcpy_s(opt.name, "PVDRTStream");
		result = syncControl->create(&rt, pvd, &opt);
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = rt; AObjectAddRef(rt);

		AMsgInit(&sm->msg, AMsgType_Option, (char*)option, sizeof(*option));
		sm->msg.done = NULL;
		result = sm->object->open(sm->object, &sm->msg);

		TRACE("open(%s) result = %d.\n", opt.name, result);
		if (result < 0) {
			HeartMsgFree(sm, result);
		} else {
			sm->reqix = 0;
			QueueUserWorkItem(&PVDRecvProc, sm, 0);
		}
	}
	return result;
}

static void PVDProxyExit(void)
{
	if (pvd != NULL) {
		pvd->close(pvd, NULL);
		AObjectRelease(pvd);
		pvd = NULL;
	}
	if (rt != NULL) {
		rt->close(rt, NULL);
		AObjectRelease(rt);
		rt = NULL;
	}
}

static long PVDProxyProbe(AObject *object, AMessage *msg)
{
	long result = PVDCmdDecode(0, msg->data, msg->size);
	if (result <= 0)
		return -1;
	return 60;
}

AModule PVDProxyModule = {
	"proxy",
	"PVDProxy",
	sizeof(PVDProxy),
	&PVDProxyInit,
	&PVDProxyExit,
	&PVDProxyCreate,
	&PVDProxyRelease,
	&PVDProxyProbe,
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};
