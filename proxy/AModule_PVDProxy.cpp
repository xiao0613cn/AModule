#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "../PVDClient/PvdNetCmd.h"
#include "../base/srsw.hpp"

static AObject *pvd = NULL;
static STRUCT_SDVR_DEVICE_EX login_data;
static STRUCT_SDVR_ALARM_EX heart_data;
static STRUCT_SDVR_INFO dvr_info;
DWORD  userid;
static AObject *rt = NULL;

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
	TRACE("%p: result = %d.\n", sm->object, result);
	release_s(sm->object, AObjectRelease, NULL);
	free(sm);
}

struct PVDProxy {
	AObject     object;
	AObject    *client;
	AMessage    inmsg;
	AMessage    outmsg;
	SliceBuffer outbuf;
	DWORD       outtick;
	long volatile reqcount;
	AMessage     *outfrom;
	srsw_queue<AMessage, 64> frame_queue;
	srsw_buffer   frame_buffer;
};
#define to_proxy(obj) container_of(obj, PVDProxy, object)
#define from_inmsg(msg) container_of(msg, PVDProxy, inmsg)
#define from_outmsg(msg) container_of(msg, PVDProxy, outmsg)

static void PVDProxyRelease(AObject *object)
{
	PVDProxy *p = to_proxy(object);
	TRACE("%p: free\n", &p->object);
	release_s(p->client, AObjectRelease, NULL);
	SliceFree(&p->outbuf);
	free(p);
}
static long PVDProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	if ((pvd == NULL) || (parent == NULL))
		return -EFAULT;

	PVDProxy *p = (PVDProxy*)malloc(sizeof(PVDProxy));
	if (p == NULL)
		return -ENOMEM;

	extern AModule PVDProxyModule;
	AObjectInit(&p->object, &PVDProxyModule);
	p->client = parent; AObjectAddRef(parent);
	SliceInit(&p->outbuf);
	p->reqcount = 0;

	*object = &p->object;
	return 1;
}

static long PVDProxyDispatch(PVDProxy *p);
static long PVDClientSendDone(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0)
		result = PVDProxyDispatch(p);
	if (result != 0)
		result = p->outfrom->done(p->outfrom, result);
	return result;
}
static long PVDProxySendDone(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (InterlockedDecrement(&p->reqcount) != 0)
		return 0;

	SlicePop(&p->outbuf, p->inmsg.size);
	if (p->outmsg.size == 0) {
		result = -EFAULT;
	} else {
		AMsgInit(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
		p->inmsg.done = &PVDClientSendDone;
		result = p->client->request(p->client, ARequest_Input, &p->inmsg);
	}
	if (result > 0)
		result = PVDProxyDispatch(p);
	if (result != 0)
		result = p->outfrom->done(p->outfrom, result);
	return result;
}
static long PVDProxyRecvDone(AMessage *msg, long result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result == 0) {
		if (p->outmsg.type == p->inmsg.type)
			return 1;
		if (long(GetTickCount()-p->outtick) < 5000) {
			AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
			return 0;
		}
		TRACE("%p: command(%02x) timeout...\n", p, p->inmsg.type&~AMsgType_Custom);
		AMsgInit(&p->outmsg, p->inmsg.type, NULL, sizeof(pvdnet_head));
		return -1;
	}
	if (SliceReserve(&p->outbuf, p->outmsg.size) < 0)
		p->outmsg.size = 0;
	if (p->outmsg.size != 0) {
		if (p->outmsg.data == NULL) {
			p->outmsg.data = SliceResPtr(&p->outbuf);
			result = PVDCmdEncode(userid, p->outmsg.data, p->outmsg.type&~AMsgType_Custom, 0);
			assert(result == p->outmsg.size);
		} else {
			memcpy(SliceResPtr(&p->outbuf), p->outmsg.data, p->outmsg.size);
			p->outmsg.data = SliceResPtr(&p->outbuf);
		}
	}
	result = PVDProxySendDone(&p->inmsg, result);
	return result;
}
static long PVDProxyRecvStream(AMessage *msg, long result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result == 0) {
		if (p->reqcount == -1)
			return -1;
		if (p->frame_queue.size() < p->frame_queue._capacity())
		{
			byte_t *ptr[2]; size_t len[2];
			p->frame_buffer.reserve(ptr, len);
			if ((len[0] < msg->size) && (len[1] >= msg->size)) {
				ptr[0] = ptr[1];
				len[0] = len[1];
			}
			if (len[0] >= msg->size) {
				memcpy(ptr[0], msg->data, msg->size);
				p->frame_buffer.commit(ptr[0], msg->size);

				AMessage &frame = p->frame_queue.end();
				AMsgInit(&frame, msg->type, (char*)ptr[0], msg->size);
				p->frame_queue.put_end();
				result = 1;
			}
		}
		if (result == 0)
			TRACE("%p: drop stream frame(%d) size = %d...\n", p, msg->type&~AMsgType_Custom, msg->size);
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}
	if (result < 0) {
		p->reqcount = -1;
		AObjectRelease(&p->object);
	}
	return result;
}
static DWORD WINAPI PVDProxySendStream(void *param)
{
	PVDProxy *p = (PVDProxy*)param;
	long result = 1;
	do {
		if (p->reqcount == -1)
			break;
		if (p->frame_queue.size() == 0) {
			Sleep(10);
			continue;
		}

		AMessage &msg = p->frame_queue.front();
		AMsgInit(&p->inmsg, AMsgType_Custom|msg.type, msg.data, msg.size);
		result = p->client->request(p->client, ARequest_Input, &p->inmsg);
		if (result <= 0)
			break;

		p->frame_buffer.decommit((unsigned char*)p->inmsg.data, p->inmsg.size);
		p->frame_queue.get_front();
	} while (result > 0);
	if (result != 0) {
		p->reqcount = -1;
		AObjectRelease(&p->object);
	}
	return result;
}
static long PVDProxyStreamDone(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0) {
		QueueUserWorkItem(PVDProxySendStream, p, 0);
	} else {
		p->reqcount = -1;
		AObjectRelease(&p->object);
	}
	return result;
}
static long PVDProxyRTStream(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0) {
		SliceReset(&p->outbuf);
		SliceResize(&p->outbuf, 2048*1024);
		p->frame_queue.reset();
		p->frame_buffer.reset((unsigned char*)p->outbuf.buf, p->outbuf.siz);

		AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
		p->outmsg.done = &PVDProxyRecvStream;

		AObjectAddRef(&p->object);
		result = rt->request(rt, ANotify_InQueueFront|0, &p->outmsg);
		if (result < 0)
			AObjectRelease(&p->object);
		else {
			p->inmsg.done = &PVDProxyStreamDone;
			AObjectAddRef(&p->object);
			QueueUserWorkItem(&PVDProxySendStream, p, 0);
			result = -1;
		}
	}
	if ((result < 0) && (p->outfrom != NULL)) {
		result = p->outfrom->done(p->outfrom, result);
	}
	return result;
}
extern long PVDTryOutput(DWORD userid, SliceBuffer *outbuf, AMessage *outmsg);
static long PVDProxyDispatch(PVDProxy *p)
{
	long result;
	do {
		p->inmsg.type = p->outfrom->type;
		result = PVDTryOutput(0, &p->outbuf, &p->inmsg);
		if (result < 0)
			return result;
		if (result == 0)
			return p->outfrom->size;

		pvdnet_head *phead = (pvdnet_head*)p->inmsg.data;
		p->inmsg.type = AMsgType_Custom|phead->uCmd;
		switch (phead->uCmd)
		{
		case NET_SDVR_MD5ID_GET:   p->outmsg.size = sizeof(pvdnet_head) + 4; break;
		case NET_SDVR_LOGIN:       p->outmsg.size = sizeof(pvdnet_head) + sizeof(login_data); break;
		case NET_SDVR_GET_DVRTYPE: p->outmsg.size = sizeof(pvdnet_head) + sizeof(dvr_info); break;
		case NET_SDVR_SHAKEHAND:   p->outmsg.size = sizeof(pvdnet_head) + sizeof(heart_data); break;
		case NET_SDVR_KEYFRAME:
		case NET_SDVR_REAL_STOP:
		case NET_SDVR_LOGOUT:      p->outmsg.size = sizeof(pvdnet_head) + 0; break;
		case NET_SDVR_REAL_PLAY:
			phead->uLen = 0;
			phead->uResult = 1;
			p->inmsg.size = sizeof(pvdnet_head) + 0;
			p->inmsg.done = &PVDProxyRTStream;
			result = p->client->request(p->client, ARequest_Input, &p->inmsg);
			if (result != 0) {
				p->outfrom = NULL;
				result = PVDProxyRTStream(&p->inmsg, result);
			}
			return result;
		default:
			assert(p->reqcount == 0);
			InterlockedExchange(&p->reqcount, 2);

			AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
			p->outmsg.done = &PVDProxyRecvDone;
			p->outtick = GetTickCount();
			result = pvd->request(pvd, ANotify_InQueueBack|ARequest_Output, &p->outmsg);
			if (result < 0) {
				InterlockedExchange(&p->reqcount, 0);
				return result;
			}

			p->inmsg.done = &PVDProxySendDone;
			result = pvd->request(pvd, ARequest_Input, &p->inmsg);
			if (result != 0) {
				if (InterlockedDecrement(&p->reqcount) != 0)
					return 0;
				SlicePop(&p->outbuf, p->inmsg.size);
				if (p->outmsg.size == 0) {
					result = -EFAULT;
				} else {
					AMsgInit(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
					p->inmsg.done = &PVDClientSendDone;
					result = p->client->request(p->client, ARequest_Input, &p->inmsg);
				}
			}
			continue;
		}

		SlicePop(&p->outbuf, p->inmsg.size);
		result = SliceReserve(&p->outbuf, p->outmsg.size);
		if (result < 0)
			break;

		p->inmsg.data = SliceResPtr(&p->outbuf);
		p->inmsg.size = PVDCmdEncode(userid, p->inmsg.data, p->inmsg.type&~AMsgType_Custom, p->outmsg.size-sizeof(pvdnet_head));
		assert(p->inmsg.size == p->outmsg.size);

		phead = (pvdnet_head*)p->inmsg.data;
		phead->uResult = 1;
		switch (phead->uCmd)
		{
		case NET_SDVR_MD5ID_GET:   p->inmsg.data[sizeof(pvdnet_head)] = 0x50; break;
		case NET_SDVR_LOGIN:       memcpy(phead+1, &login_data, sizeof(login_data)); break;
		case NET_SDVR_GET_DVRTYPE: memcpy(phead+1, &dvr_info, sizeof(dvr_info)); break;
		case NET_SDVR_SHAKEHAND:   memcpy(phead+1, &heart_data, sizeof(heart_data)); break;
		case NET_SDVR_KEYFRAME:
		case NET_SDVR_REAL_STOP:
		case NET_SDVR_LOGOUT:
			break;
		default:
			assert(FALSE);
			break;
		}

		p->inmsg.done = &PVDClientSendDone;
		result = p->client->request(p->client, ARequest_Input, &p->inmsg);
	} while (result > 0);
	return result;
}

static long PVDProxyOpen(AObject *object, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AObject)))
		return -EINVAL;

	release_s(p->client, AObjectRelease, NULL);
	p->client = (AObject*)msg->data;
	AObjectAddRef(p->client);
	return 1;
}

static long PVDProxyRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if (reqix != ARequest_Input)
		return -ENOSYS;

	long result = SliceReserve(&p->outbuf, max(msg->size,2048));
	if (result < 0)
		return result;

	if (msg->data == NULL) {
		AMsgInit(msg, AMsgType_Unknown, SliceResPtr(&p->outbuf), SliceResLen(&p->outbuf));
		return 1;
	}

	if (msg->data != SliceResPtr(&p->outbuf))
		memcpy(SliceResPtr(&p->outbuf), msg->data, msg->size);
	SlicePush(&p->outbuf, msg->size);

	p->outfrom = msg;
	result = PVDProxyDispatch(p);
	return result;
}

//////////////////////////////////////////////////////////////////////////
static long PVDSendDone(AMessage *msg, long result);
static DWORD WINAPI PVDSendProc(void *p)
{
	HeartMsg *sm = (HeartMsg*)p;
	long result;
	do {
		if (sm->msg.type == AMsgType_Option) {
			result = NET_SDVR_GET_DVRTYPE;
		} else {
			::Sleep(3000);
			result = NET_SDVR_SHAKEHAND;
		}
		sm->msg.type = AMsgType_Custom|result;
		sm->msg.data = (char*)&sm->heart;
		sm->msg.size = PVDCmdEncode(0, &sm->heart, result, 0);
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
		pvdnet_head *phead = (pvdnet_head*)msg->data;
		if (phead->uFlag == NET_CMD_HEAD_FLAG) {
			switch (phead->uCmd)
			{
			case NET_SDVR_SHAKEHAND:
				memcpy(&heart_data, phead+1, min(sizeof(heart_data),msg->size-sizeof(pvdnet_head)));
				break;
			case NET_SDVR_GET_DVRTYPE:
				memcpy(&dvr_info, phead+1, min(sizeof(dvr_info),msg->size-sizeof(pvdnet_head)));
				break;
			}
		}
		/*MSHEAD *mshead = (MSHEAD*)msg->data;
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
			strcpy_s(opt.name, "login_data");
			opt.extend = &login_data;
			pvd->getopt(pvd, &opt);

			strcpy_s(opt.name, "session_id");
			pvd->getopt(pvd, &opt);
			userid = atol(opt.value);

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
		opt.value[0] = '\0';
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
	&PVDProxyOpen,
	NULL,
	NULL,
	&PVDProxyRequest,
	NULL,
	NULL,
};
