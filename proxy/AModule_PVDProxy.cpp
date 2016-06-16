#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../base/SliceBuffer.h"
#include "../io/AModule_io.h"
#include "../PVDClient/PvdNetCmd.h"
#include "../base/srsw.hpp"


static AObject *pvd = NULL;
static STRUCT_SDVR_DEVICE_EX login_data;
static STRUCT_SDVR_ALARM_EX heart_data;
static STRUCT_SDVR_INFO dvr_info;
static STRUCT_SDVR_SUPPORT_FUNC supp_func;
static STRUCT_SDVR_DEVICEINFO devcfg_info;
static STRUCT_SDVR_NETINFO netcfg_info;
static STRUCT_SDVR_DEVICEINFO_EX devinfo_ex;
DWORD  userid;
static DWORD rt_active = 0;
static BOOL force_alarm = FALSE;

AObject *rt = NULL;
AMessage rt_msg = { 0 };

#pragma warning(disable: 4201)
struct HeartMsg {
	AMessage msg;
	AObject *object;
	AOption *option;
	AOperator timer;
	union {
	pvdnet_head heart;
	struct {
	long        reqix;
	long        threadix;
	};
	};
};
#pragma warning(default: 4201)

static void HeartMsgFree(HeartMsg *sm, long result)
{
	TRACE("%p: result = %d.\n", sm->object, result);
	release_s(sm->object, aobject_release, NULL);
	release_s(sm->option, aoption_release, NULL);
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
	srsw_queue<AMessage,64> frame_queue;
	AOperator timer;
};
#define to_proxy(obj) container_of(obj, PVDProxy, object)
#define from_inmsg(msg) container_of(msg, PVDProxy, inmsg)
#define from_outmsg(msg) container_of(msg, PVDProxy, outmsg)

static void PVDProxyRelease(AObject *object)
{
	PVDProxy *p = to_proxy(object);
	//TRACE("%p: free\n", &p->object);
	release_s(p->client, aobject_release, NULL);
	SliceFree(&p->outbuf);

	while (p->frame_queue.size() != 0) {
		RTBufferFree(RTMsgGet(&p->frame_queue.front()));
		p->frame_queue.get_front();
	}
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
	aobject_init(&p->object, &PVDProxyModule);
	p->client = parent;
	if (parent != NULL)
		aobject_addref(parent);
	SliceInit(&p->outbuf);
	p->reqcount = 0;
	p->frame_queue.reset();

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
		amsg_init(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
		p->inmsg.done = &PVDClientSendDone;
		result = p->client->request(p->client, Aio_Input, &p->inmsg);
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
			amsg_init(&p->outmsg, AMsgType_Unknown, NULL, 0);
			return 0;
		}
		TRACE("command(%02x) timeout...\n", p->inmsg.type&~AMsgType_Custom);
		amsg_init(&p->outmsg, p->inmsg.type, NULL, sizeof(pvdnet_head));
		return -1;
	}
	if (SliceReserve(&p->outbuf, p->outmsg.size, 2048) < 0)
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
	if (result == 0)
	{
		// on notify callback
		if ((p->reqcount == -1) || (rt_msg.data == NULL))
			return -1;

		if (p->frame_queue.size() < p->frame_queue._capacity()) {
			RTBufferAddRef(RTMsgGet(&rt_msg));
			p->frame_queue.put_back(rt_msg);
		} else {
			if (long(GetTickCount()-p->outtick) > 15000)
				return -1;
			TRACE("drop stream frame(%d) size = %d...\n",
				msg->type&~AMsgType_Custom, msg->size);
		}
		amsg_init(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}
	if (result < 0) {
		p->reqcount = -1;
		aobject_release(&p->object);
	}
	return result;
}
static void PVDProxySendStream(AOperator *asop, int result)
{
	PVDProxy *p = container_of(asop, PVDProxy, timer);
	if (result >= 0)
	do {
		if (p->reqcount == -1) {
			result = -1;
			break;
		}
		p->outtick = GetTickCount();
		if (p->frame_queue.size() == 0) {
			aoperator_timewait(&p->timer, NULL, 10);
			return;
		}

		AMessage &frame = p->frame_queue.front();
		amsg_init(&p->inmsg, AMsgType_Custom|frame.type, frame.data, frame.size);
		result = p->client->request(p->client, Aio_Input, &p->inmsg);
		if (result <= 0)
			break;

		RTBufferFree(RTMsgGet(&frame));
		p->frame_queue.get_front();
	} while (result > 0);
	if (result != 0) {
		p->reqcount = -1;
		aobject_release(&p->object);
	}
}
static long PVDProxyStreamDone(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0) {
		RTBufferFree(RTMsgGet(&p->frame_queue.front()));
		p->frame_queue.get_front();
		PVDProxySendStream(&p->timer, 1);
	} else {
		p->reqcount = -1;
		aobject_release(&p->object);
	}
	return result;
}
static long PVDProxyRTStream(AMessage *msg, long result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result >= 0) {
		p->frame_queue.reset();
		amsg_init(&p->outmsg, AMsgType_Unknown, NULL, 0);
		p->outmsg.done = &PVDProxyRecvStream;

		aobject_addref(&p->object);
		result = rt->request(rt, Aiosync_NotifyBack|0, &p->outmsg);
		if (result < 0) {
			aobject_release(&p->object);
		} else {
			p->inmsg.done = &PVDProxyStreamDone;
			p->timer.callback = &PVDProxySendStream;
			aobject_addref(&p->object);
			aoperator_timewait(&p->timer, NULL, 0);
			result = -1;
		}
	}
	if (p->outfrom != NULL) {
		result = p->outfrom->done(p->outfrom, -1);
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
		case NET_SDVR_SUPPORT_FUNC: p->outmsg.size = sizeof(pvdnet_head) + sizeof(supp_func); break;
		case NET_SDVR_DEVICECFG_GET: p->outmsg.size = sizeof(pvdnet_head) + sizeof(devcfg_info); break;
		case NET_SDVR_DEVICECFG_GET_EX: p->outmsg.size = sizeof(pvdnet_head) + sizeof(devinfo_ex); break;
		case NET_SDVR_NETCFG_GET:  p->outmsg.size = sizeof(pvdnet_head) + sizeof(netcfg_info); break;
		case NET_SDVR_SHAKEHAND:   p->outmsg.size = sizeof(pvdnet_head) + sizeof(heart_data); break;
		case NET_SDVR_KEYFRAME:
		case NET_SDVR_REAL_STOP:
		case NET_SDVR_LOGOUT:      p->outmsg.size = sizeof(pvdnet_head) + 0; break;
		case NET_SDVR_REAL_PLAY:
		case NETCOM_VOD_RECFILE_REQ:
		case NETCOM_VOD_RECFILE_REQ_EX:
			phead->uLen = 0;
			phead->uResult = 1;
			p->inmsg.size = sizeof(pvdnet_head) + 0;
			p->inmsg.done = &PVDProxyRTStream;
			p->outtick = GetTickCount();
			result = p->client->request(p->client, Aio_Input, &p->inmsg);
			if (result != 0) {
				p->outfrom = NULL;
				result = PVDProxyRTStream(&p->inmsg, result);
			}
			return result;
		default:
			assert(p->reqcount == 0);
			InterlockedExchange(&p->reqcount, 2);

			amsg_init(&p->outmsg, AMsgType_Unknown, NULL, 0);
			p->outmsg.done = &PVDProxyRecvDone;
			p->outtick = GetTickCount();
			result = pvd->request(pvd, Aiosync_NotifyBack|Aio_Output, &p->outmsg);
			if (result < 0) {
				InterlockedExchange(&p->reqcount, 0);
				return result;
			}

			p->inmsg.done = &PVDProxySendDone;
			result = pvd->request(pvd, Aio_Input, &p->inmsg);
			if (result == 0)
				return 0;
			if (InterlockedDecrement(&p->reqcount) != 0)
				return 0;
			SlicePop(&p->outbuf, p->inmsg.size);
			if (p->outmsg.size == 0)
				return -EFAULT;
			amsg_init(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
			p->inmsg.done = &PVDClientSendDone;
			result = p->client->request(p->client, Aio_Input, &p->inmsg);
			continue;
		}

		SlicePop(&p->outbuf, p->inmsg.size);
		result = SliceReserve(&p->outbuf, p->outmsg.size, 2048);
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
		case NET_SDVR_SUPPORT_FUNC: memcpy(phead+1, &supp_func, sizeof(supp_func)); break;
		case NET_SDVR_DEVICECFG_GET: memcpy(phead+1, &devcfg_info, sizeof(devcfg_info)); break;
		case NET_SDVR_DEVICECFG_GET_EX: memcpy(phead+1, &devinfo_ex, sizeof(devinfo_ex)); break;
		case NET_SDVR_NETCFG_GET:  memcpy(phead+1, &netcfg_info, sizeof(netcfg_info)); break;
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
		result = p->client->request(p->client, Aio_Input, &p->inmsg);
	} while (result > 0);
	return result;
}

static long PVDProxyOpen(AObject *object, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	release_s(p->client, aobject_release, NULL);
	p->client = (AObject*)msg->data;
	aobject_addref(p->client);
	return 1;
}

static long PVDProxyRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if (reqix != Aio_Input)
		return -ENOSYS;

	long result = SliceReserve(&p->outbuf, max(msg->size,1024), 2048);
	if (result < 0)
		return result;

	if (msg->data == NULL) {
		amsg_init(msg, AMsgType_Unknown, SliceResPtr(&p->outbuf), SliceResLen(&p->outbuf));
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
static void PVDDoSend(AOperator *asop, int result)
{
	HeartMsg *sm = container_of(asop, HeartMsg, timer);
	if ((result < 0) || (pvd == NULL)) {
		HeartMsgFree(sm, result);
		return;
	}
	DWORD tick = GetTickCount();
	if (long(tick-rt_active) > 10*1000) {
		TRACE("realtime timeout...\n");
		rt->close(rt, NULL);
		rt_active = tick;
	}
	do {
		switch (sm->msg.type)
		{
		case AMsgType_Option: result = NET_SDVR_GET_DVRTYPE; break;
		case (AMsgType_Custom|NET_SDVR_GET_DVRTYPE): result = NET_SDVR_SUPPORT_FUNC; break;
		case (AMsgType_Custom|NET_SDVR_SUPPORT_FUNC): result = NET_SDVR_DEVICECFG_GET; break;
		case (AMsgType_Custom|NET_SDVR_DEVICECFG_GET): result = NET_SDVR_DEVICECFG_GET_EX; break;
		case (AMsgType_Custom|NET_SDVR_DEVICECFG_GET_EX): result = NET_SDVR_NETCFG_GET; break;
		default:
			if (sm->msg.type != (AMsgType_Custom|NET_SDVR_SHAKEHAND)) {
				result = NET_SDVR_SHAKEHAND;
				break;
			}
			sm->msg.type = 0;
			aoperator_timewait(&sm->timer, NULL, 3*1000);
			return;
		}
		sm->msg.type = AMsgType_Custom|result;
		sm->msg.data = (char*)&sm->heart;
		sm->msg.size = PVDCmdEncode(0, &sm->heart, result, 0);
		result = sm->object->request(sm->object, Aio_Input, &sm->msg);
	} while (result > 0);
	if (result < 0) {
		sm->msg.type = AMsgType_Option;
		aoperator_timewait(&sm->timer, NULL, 3*1000);
	}
}
static long PVDSendDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result < 0) {
		sm->msg.type = AMsgType_Option;
	}
	aoperator_timewait(&sm->timer, NULL, 3*1000);
	return result;
}
static void PVDDoOpen(AOperator *asop, int result);
static long PVDCloseDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (pvd == NULL) {
		HeartMsgFree(sm, result);
	} else {
		sm->timer.callback = &PVDDoOpen;
		aoperator_timewait(&sm->timer, NULL, 10*1000);
	}
	return result;
}
static void PVDDoClose(HeartMsg *sm, long result)
{
	TRACE("%s result = %d.\n", (sm->object==pvd)?"client":"realtime", result);

	amsg_init(&sm->msg, AMsgType_Unknown, NULL, 0);
	sm->msg.done = &PVDCloseDone;
	result = sm->object->close(sm->object, &sm->msg);
	if (result != 0)
		PVDCloseDone(&sm->msg, result);
}
static long PVDRecvDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result < 0) {
		PVDDoClose(sm, result);
		return result;
	}

	pvdnet_head *phead = (pvdnet_head*)msg->data;
	if ((phead->uFlag == NET_CMD_HEAD_FLAG) && (phead->uResult != 0)) {
		void *ptr = NULL;
		int len = 0;

		switch (phead->uCmd)
		{
		case NET_SDVR_SHAKEHAND:
			if (!force_alarm) {
				ptr = &heart_data; len = sizeof(heart_data);
			} else {
				memset(heart_data.wMotion, force_alarm, sizeof(heart_data.wMotion));
				memset(heart_data.wAlarm, force_alarm, sizeof(heart_data.wAlarm));
				memset(heart_data.byDisk, force_alarm, sizeof(heart_data.byDisk));
			}
			break;
		case NET_SDVR_GET_DVRTYPE: ptr = &dvr_info; len = sizeof(dvr_info); break;
		case NET_SDVR_SUPPORT_FUNC: ptr = &supp_func; len = sizeof(supp_func); break;
		case NET_SDVR_DEVICECFG_GET: ptr = &devcfg_info; len = sizeof(devcfg_info); break;
		case NET_SDVR_DEVICECFG_GET_EX: ptr = &devinfo_ex; len = sizeof(devinfo_ex); break;
		case NET_SDVR_NETCFG_GET: ptr = &netcfg_info; len = sizeof(netcfg_info); break;
		}
		if (ptr != NULL) {
			memcpy(ptr, phead+1, min(len,msg->size-sizeof(pvdnet_head)));
		}
	}
	/*MSHEAD *mshead = (MSHEAD*)msg->data;
	STREAM_HEADER *shead = (STREAM_HEADER*)msg->data;
	if ((phead->uFlag == NET_CMD_HEAD_FLAG)
	 || (ISMSHEAD(mshead) && ISKEYFRAME(mshead))
	 || (shead->nHeaderFlag == STREAM_HEADER_FLAG && shead->nFrameType == STREAM_FRAME_VIDEO_I))
		TRACE("result = %d.\n", result);*/
	if (sm->object == rt) {
		RTBuffer *buffer;
		long offset;
		rt_active = GetTickCount();

		if (rt_msg.data == NULL) {
			buffer = RTBufferAlloc(max(msg->size*10,1024*1024));
			offset = 0;
		} else {
			buffer = RTMsgGet(&rt_msg);
			offset = rt_msg.type + _align_8bytes(rt_msg.size);

			if (buffer->size < offset+msg->size) {
				RTBufferFree(buffer);
				buffer = RTBufferAlloc(max(msg->size*10,1024*1024));
				offset = 0;
			}
		}
		RTMsgSet(&rt_msg, buffer, offset);
		memcpy(rt_msg.data, msg->data, msg->size);
		rt_msg.size = msg->size;
	}
	aoperator_timewait(&sm->timer, NULL, 0);
	return result;
}
static void PVDDoRecv(AOperator *asop, int result)
{
	HeartMsg *sm = container_of(asop, HeartMsg, timer);
	if ((result < 0) || (pvd == NULL)) {
		HeartMsgFree(sm, result);
		return;
	}

	amsg_init(&sm->msg, AMsgType_Unknown, NULL, 0);
	result = sm->object->request(sm->object, sm->reqix, &sm->msg);
	if (result != 0) {
		sm->msg.done(&sm->msg, result);
	}
}

static long PVDOpenDone(AMessage *msg, long result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result < 0) {
		PVDDoClose(sm, result);
		return result;
	}

	TRACE("%s result = %d.\n", (sm->object==pvd)?"client":"realtime", result);
	if (sm->object == pvd) {
		AOption opt;
		aoption_init(&opt, NULL);

		strcpy_s(opt.name, "login_data");
		opt.extend = &login_data;
		sm->object->getopt(sm->object, &opt);

		strcpy_s(opt.name, "session_id");
		sm->object->getopt(sm->object, &opt);
		userid = atol(opt.value);

		AOption *opt2 = aoption_find_child(sm->option, "channel_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byChanNum = atol(opt2->value);

		opt2 = aoption_find_child(sm->option, "alarm_in_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byAlarmInPortNum = atol(opt2->value);

		opt2 = aoption_find_child(sm->option, "alarm_out_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byAlarmOutPortNum = atol(opt2->value);

		opt2 = aoption_find_child(sm->option, "hdd_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byDiskNum = atol(opt2->value);
	} else {
		rt_active = GetTickCount();
	}

	sm->msg.done = &PVDRecvDone;
	sm->timer.callback = &PVDDoRecv;
	aoperator_timewait(&sm->timer, NULL, 0);
	return result;
}

static void PVDDoOpen(AOperator *asop, int result)
{
	HeartMsg *sm = container_of(asop, HeartMsg, timer);
	if ((result < 0) || (pvd == NULL)) {
		HeartMsgFree(sm, result);
		return;
	}

	if (sm->object == rt) {
		AOption opt;
		aoption_init(&opt, NULL);

		strcpy_s(opt.name, "version");
		_ltoa_s(login_data.byDVRType, opt.value, 10);
		sm->object->setopt(sm->object, &opt);

		strcpy_s(opt.name, "session_id");
		_ltoa_s(userid, opt.value, 10);
		sm->object->setopt(sm->object, &opt);
	}

	amsg_init(&sm->msg, AMsgType_Option, (char*)sm->option, 0);
	sm->msg.done = &PVDOpenDone;

	result = sm->object->open(sm->object, &sm->msg);
	if (result != 0)
		PVDOpenDone(&sm->msg, result);
}

long PVDProxyInit(AOption *option)
{
	if (option == NULL)
		return 0;
	if (_stricmp(option->name, "stream") != 0)
		return 0;

	AOption *opt2 = aoption_find_child(option, "force_alarm");
	if (opt2 != NULL)
		force_alarm = atoi(opt2->value);

	long result = -EFAULT;
	AOption opt;
	aoption_init(&opt, NULL);

	AModule *syncControl = amodule_find("stream", "SyncControl");
	if (syncControl != NULL) {
		strcpy_s(opt.name, "stream");
		strcpy_s(opt.value, option->value);
		result = syncControl->create(&pvd, NULL, &opt);
	}
	HeartMsg *sm = NULL;
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; aobject_addref(pvd);
		sm->option = NULL;

		sm->msg.type = AMsgType_Option;
		sm->msg.done = &PVDSendDone;
		sm->timer.callback = &PVDDoSend;
		aoperator_timewait(&sm->timer, NULL, 3*1000);
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; aobject_addref(pvd);
		sm->option = aoption_clone(option);
		sm->reqix = Aio_Output;
		sm->threadix = 1;
		sm->timer.callback = &PVDDoOpen;
		aoperator_timewait(&sm->timer, NULL, 0);

		strcpy_s(opt.value, "PVDRTStream");
		result = syncControl->create(&rt, NULL, &opt);
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = rt; aobject_addref(rt);
		sm->option = aoption_clone(option);
		sm->reqix = 0;
		sm->threadix = 2;
		sm->timer.callback = &PVDDoOpen;
		aoperator_timewait(&sm->timer, NULL, 3*1000);
	}
	return result;
}

static void PVDProxyExit(void)
{
	if (pvd != NULL) {
		//pvd->cancel(pvd, ARequest_MsgLoop|Aio_Output, NULL);
		pvd->close(pvd, NULL);
		aobject_release(pvd);
		pvd = NULL;
	}
	if (rt != NULL) {
		//rt->cancel(rt, ARequest_MsgLoop|0, NULL);
		rt->close(rt, NULL);
		aobject_release(rt);
		rt = NULL;
	}
	if (rt_msg.data != NULL) {
		RTBufferFree(RTMsgGet(&rt_msg));
		rt_msg.data = NULL;
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
