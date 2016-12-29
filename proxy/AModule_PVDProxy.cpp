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
static STRUCT_SDVR_REIPCWORKPARAM ipc_work;
static STRUCT_SDVR_WORKSTATE_EX work_status;
static STRUCT_SDVR_DEVICEINFO_EX devinfo_ex;
static DWORD  userid;
static DWORD rt_active = 0;
static BOOL force_alarm = FALSE;

static AOption *proactive_option = NULL;
static AOption *proactive_io = NULL;
static const char *proactive_prefix = "";
static long volatile proactive_first = 0;

AObject *rt = NULL;
ARefsMsg rt_msg = { 0 };

#pragma warning(disable: 4201)
struct HeartMsg {
	AMessage msg;
	AObject *object;
	AOption *option;
	AOperator timer;
	pvdnet_head heart;
	union {
	struct {
	int     reqix;
	int     threadix;
	};
	STRUCT_SDVR_REQIPCWORKPARAM ipcreq;
	};
};
#pragma warning(default: 4201)

static void HeartMsgFree(HeartMsg *sm, int result)
{
	TRACE("%p: result = %d.\n", sm->object, result);
	release_s(sm->object, AObjectRelease, NULL);
	release_s(sm->option, AOptionRelease, NULL);
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
	srsw_queue<ARefsMsg,64> frame_queue;
	AOperator timer;
	pvdnet_head  null_cmd;
	long volatile proactive_id;
};
#define to_proxy(obj) container_of(obj, PVDProxy, object)
#define from_inmsg(msg) container_of(msg, PVDProxy, inmsg)
#define from_outmsg(msg) container_of(msg, PVDProxy, outmsg)

static void PVDProxyRelease(AObject *object)
{
	PVDProxy *p = to_proxy(object);
	//TRACE("%p: free\n", &p->object);
	release_s(p->client, AObjectRelease, NULL);
	SliceFree(&p->outbuf);

	while (p->frame_queue.size() != 0) {
		ARefsBufRelease(p->frame_queue.front().buf);
		p->frame_queue.get_front();
	}
}
static int PVDProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	if ((pvd == NULL) /*|| (parent == NULL)*/)
		return -EFAULT;

	PVDProxy *p = (PVDProxy*)*object;
	p->client = parent;
	if (parent != NULL)
		AObjectAddRef(parent);

	SliceInit(&p->outbuf);
	p->reqcount = 0;
	p->frame_queue.reset();
	p->proactive_id = 0;
	return 1;
}

static int PVDProxyDispatch(PVDProxy *p);
static int PVDClientSendDone(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0)
		result = PVDProxyDispatch(p);
	if (result != 0)
		result = p->outfrom->done(p->outfrom, result);
	return result;
}
static int PVDProxySendDone(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	if (InterlockedAdd(&p->reqcount, -1) != 0)
		return 0;

	SlicePop(&p->outbuf, p->inmsg.size);
	if (p->outmsg.size == 0) {
		result = -EFAULT;
	} else {
		AMsgInit(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
		p->inmsg.done = &PVDClientSendDone;
		result = ioInput(p->client, &p->inmsg);
	}
	if (result > 0)
		result = PVDProxyDispatch(p);
	if (result != 0)
		result = p->outfrom->done(p->outfrom, result);
	return result;
}
static int PVDProxyRecvDone(AMessage *msg, int result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result == 0) {
		if (p->outmsg.type == p->inmsg.type)
			return 1;
		if (int(GetTickCount()-p->outtick) < 5000) {
			AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
			return 0;
		}
		pvdnet_head *phead = (pvdnet_head*)p->inmsg.data;
		TRACE("command(%02x) timeout...\n", phead->uCmd);
		AMsgInit(&p->outmsg, p->inmsg.type, NULL, sizeof(pvdnet_head));
		return -1;
	}
	if (SliceReserve(&p->outbuf, p->outmsg.size, 2048) < 0)
		p->outmsg.size = 0;
	if (p->outmsg.size != 0) {
		if (p->outmsg.data == NULL) {
			p->outmsg.data = SliceResPtr(&p->outbuf);
			result = PVDCmdEncode(userid, p->outmsg.data, p->outmsg.type&~AMsgType_Private, 0);
			assert(result == p->outmsg.size);
		} else {
			memcpy(SliceResPtr(&p->outbuf), p->outmsg.data, p->outmsg.size);
			p->outmsg.data = SliceResPtr(&p->outbuf);
		}
	}
	result = PVDProxySendDone(&p->inmsg, result);
	return result;
}
static int PVDProxyRecvStream(AMessage *msg, int result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result == 0)
	{
		// on notify callback
		if ((p->reqcount == -1) || (rt_msg.buf == NULL))
			return -1;

		if (p->frame_queue.size() < p->frame_queue._capacity()) {
			ARefsBufAddRef(rt_msg.buf);
			p->frame_queue.put_back(rt_msg);
		} else {
			if (int(GetTickCount()-p->outtick) > 15000)
				return -1;
			TRACE("drop stream frame(%d) size = %d...\n",
				rt_msg.type&~AMsgType_Private, rt_msg.size);
		}
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}
	if (result < 0) {
		p->reqcount = -1;
		AObjectRelease(&p->object);
	}
	return result;
}
static int PVDProxyCloseStream(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	AObjectRelease(&p->object);
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
			AOperatorTimewait(&p->timer, NULL, 10);
			return;
		}

		ARefsMsg &frame = p->frame_queue.front();
		AMsgInit(&p->inmsg, AMsgType_Private|frame.type, frame.ptr(), frame.size);

		result = ioInput(p->client, &p->inmsg);
		if (result <= 0)
			break;

		ARefsBufRelease(frame.buf);
		p->frame_queue.get_front();
	} while (result > 0);
	if (result != 0) {
		p->reqcount = -1;

		AMsgInit(&p->inmsg, AMsgType_Unknown, NULL, 0);
		p->inmsg.done = &PVDProxyCloseStream;
		result = p->client->close(p->client, &p->inmsg);
		if (result != 0)
			result = p->inmsg.done(&p->inmsg, result);
	}
}
static int PVDProxyStreamDone(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result > 0) {
		ARefsBufRelease(p->frame_queue.front().buf);
		p->frame_queue.get_front();

		PVDProxySendStream(&p->timer, 1);
	} else {
		p->reqcount = -1;
		AObjectRelease(&p->object);
	}
	return result;
}
static int PVDProxyRTStream(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	if (result >= 0) {
		p->frame_queue.reset();
		p->outtick = GetTickCount();

		AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
		p->outmsg.done = &PVDProxyRecvStream;

		AObjectAddRef(&p->object);
		result = rt->request(rt, Aiosync_NotifyBack|0, &p->outmsg);
		if (result < 0) {
			AObjectRelease(&p->object);
		} else {
			p->inmsg.done = &PVDProxyStreamDone;
			p->timer.callback = &PVDProxySendStream;
			AObjectAddRef(&p->object);
			AOperatorTimewait(&p->timer, NULL, 0);
			result = -1;
		}
	}
	if (p->outfrom != NULL) {
		result = p->outfrom->done(p->outfrom, -1);
	}
	return result;
}

static int PVDProactiveRTPlay(AMessage *msg, int result)
{
	PVDProxy *p = from_inmsg(msg);
	result = PVDProxyRTStream(msg, result);
	AObjectRelease(&p->object);
	return result;
}

static int PVDProactiveRTConnect(AMessage *msg, int result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result < 0) {
		AObjectRelease(&p->object);
		return result;
	}

	result = PVDCmdEncode(userid, &p->null_cmd, NET_SDVR_REAL_PLAY_EX, sizeof(long));
	AMsgInit(&p->inmsg, AMsgType_Private|NET_SDVR_REAL_PLAY_EX, &p->null_cmd, result);

	p->inmsg.done = &PVDProactiveRTPlay;
	p->outfrom = NULL;

	result = ioInput(p->client, &p->inmsg);
	if (result != 0)
		PVDProactiveRTPlay(&p->inmsg, result);
	return result;
}

static int PVDProactiveRTStream(PVDProxy *p)
{
	AObject *io;
	int result = AObjectCreate(&io, NULL, proactive_io, NULL);
	if (result < 0)
		return result;

	AObject *obj;
	result = PVDProxyCreate(&obj, io, NULL);
	AObjectRelease(io);
	if (result < 0)
		return result;

	PVDProxy *p_rt; p_rt = to_proxy(obj);
	p_rt->proactive_id = ((STRUCT_SDVR_REALPLAY_INITIATIVE*)(p->inmsg.data+sizeof(pvdnet_head)))->msgid;

	AMsgInit(&p_rt->outmsg, AMsgType_Option, proactive_io, 0);
	p_rt->outmsg.done = &PVDProactiveRTConnect;

	result = p_rt->client->open(p_rt->client, &p_rt->outmsg);
	if (result != 0)
		result = PVDProactiveRTConnect(&p_rt->outmsg, result);
	return result;
}

extern int PVDTryOutput(DWORD userid, SliceBuffer *outbuf, AMessage *outmsg);
static int PVDProxyDispatch(PVDProxy *p)
{
	int result;
	do {
		p->inmsg.type = p->outfrom->type;
		result = PVDTryOutput(0, &p->outbuf, &p->inmsg);
		if (result < 0)
			return result;
		if (result == 0)
			return p->outfrom->size;

		pvdnet_head *phead = (pvdnet_head*)p->inmsg.data;
		p->inmsg.type = AMsgType_Private|phead->uCmd;
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
		case NET_SDVR_IPCWORKPARAM_GET: p->outmsg.size = sizeof(pvdnet_head) + sizeof(ipc_work); break;
		case NET_SDVR_KEYFRAME:
		case NET_SDVR_REAL_STOP:
		case NET_SDVR_LOGOUT:      p->outmsg.size = sizeof(pvdnet_head) + 0; break;
		case NET_SDVR_WORK_STATE: p->outmsg.size = sizeof(pvdnet_head) + sizeof(STRUCT_SDVR_WORKSTATE_EX); break;
		case NET_SDVR_REAL_PLAY:
		case NETCOM_VOD_RECFILE_REQ:
		case NETCOM_VOD_RECFILE_REQ_EX:
			phead->uLen = 0;
			phead->uResult = 1;
			p->inmsg.size = sizeof(pvdnet_head) + 0;
			p->inmsg.done = &PVDProxyRTStream;

			result = ioInput(p->client, &p->inmsg);
			if (result != 0) {
				p->outfrom = NULL;
				result = PVDProxyRTStream(&p->inmsg, result);
			}
			return result;

		case NET_SDVR_INITIATIVE_LOGIN:
			if (p->inmsg.size < sizeof(pvdnet_head)+sizeof(STRUCT_SDVR_INITIATIVE_LOGIN)) {
				SlicePop(&p->outbuf, p->inmsg.size);
				continue;
			}
			p->outmsg.size = sizeof(pvdnet_head);
			break;

		case NET_SDVR_REAL_PLAY_EX:
			PVDProactiveRTStream(p);
			SlicePop(&p->outbuf, p->inmsg.size);
			continue;

		default:
			assert(p->reqcount == 0);
			InterlockedExchange(&p->reqcount, 2);

			AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
			p->outmsg.done = &PVDProxyRecvDone;
			p->outtick = GetTickCount();
			result = pvd->request(pvd, Aiosync_NotifyBack|Aio_Output, &p->outmsg);
			if (result < 0) {
				InterlockedExchange(&p->reqcount, 0);
				return result;
			}

			p->inmsg.done = &PVDProxySendDone;
			result = ioInput(pvd, &p->inmsg);
			if (result == 0)
				return 0;
			if (InterlockedAdd(&p->reqcount, -1) != 0)
				return 0;
			SlicePop(&p->outbuf, p->inmsg.size);
			if (p->outmsg.size == 0)
				return -EFAULT;
			AMsgInit(&p->inmsg, p->outmsg.type, p->outmsg.data, p->outmsg.size);
			p->inmsg.done = &PVDClientSendDone;
			result = ioInput(p->client, &p->inmsg);
			continue;
		}

		SlicePop(&p->outbuf, p->inmsg.size);
		result = SliceReserve(&p->outbuf, p->outmsg.size, 2048);
		if (result < 0)
			break;

		p->inmsg.data = SliceResPtr(&p->outbuf);
		p->inmsg.size = PVDCmdEncode(userid, p->inmsg.data, p->inmsg.type&~AMsgType_Private, p->outmsg.size-sizeof(pvdnet_head));
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
		case NET_SDVR_IPCWORKPARAM_GET: memcpy(phead+1, &ipc_work, sizeof(ipc_work)); break;
		case NET_SDVR_KEYFRAME:
		case NET_SDVR_REAL_STOP:
		case NET_SDVR_LOGOUT:
		case NET_SDVR_INITIATIVE_LOGIN:
			break;
		case NET_SDVR_WORK_STATE: memcpy(phead+1, &work_status, sizeof(work_status)); break;
		default:
			assert(FALSE);
			break;
		}

		p->inmsg.done = &PVDClientSendDone;
		result = ioInput(p->client, &p->inmsg);
	} while (result > 0);
	return result;
}

static int PVDProactiveOpenStatus(PVDProxy *p, int result)
{
	if (p->outmsg.type != AMsgType_Option)
		return 1;

	if (p->proactive_id == 0)
		p->proactive_id = InterlockedAdd(&proactive_first, 1) - 1;

	SliceReset(&p->outbuf);
	result = SliceResize(&p->outbuf, max(sizeof(pvdnet_head)+sizeof(STRUCT_SDVR_INITIATIVE_LOGIN),8192), 2048);
	if (result < 0)
		return result;

	p->outmsg.type = AMsgType_Private|NET_SDVR_INITIATIVE_LOGIN;
	p->outmsg.data = SliceResPtr(&p->outbuf);
	p->outmsg.size = PVDCmdEncode(userid, p->outmsg.data, NET_SDVR_INITIATIVE_LOGIN, sizeof(STRUCT_SDVR_INITIATIVE_LOGIN));

	STRUCT_SDVR_INITIATIVE_LOGIN *login = (STRUCT_SDVR_INITIATIVE_LOGIN*)(p->outmsg.data + sizeof(pvdnet_head));
	snprintf(login->sDVRID, sizeof(login->sDVRID), "%s%ld", proactive_prefix, p->proactive_id);
	if (login_data.sSerialNumber[0] != '\0')
		strcpy_sz(login->sSerialNumber, login_data.sSerialNumber);
	else
		strcpy_sz(login->sSerialNumber, (char*)devcfg_info.sSerialNumber);
	login->byAlarmInPortNum = login_data.byAlarmInPortNum;
	login->byAlarmOutPortNum = login_data.byAlarmOutPortNum;
	login->byDiskNum = login_data.byDiskNum;
	login->byProtocol = PROTOCOL_V3;
	login->byChanNum = login_data.byChanNum;
	login->byEncodeType = login_data.byStartChan;
	memset(login->reserve, 0, sizeof(login->reserve));
	strcpy_sz(login->sDvrName, login_data.szDvrName);
	memcpy(login->sChanName, login_data.szChanName, min(sizeof(login->sChanName), sizeof(login_data.szChanName)));

	result = ioInput(p->client, &p->outmsg);
	return result;
}

static int PVDProactiveOpenDone(AMessage *msg, int result)
{
	PVDProxy *p = from_outmsg(msg);
	if (result >= 0)
		result = PVDProactiveOpenStatus(p, result);
	if (result != 0)
		result = p->outfrom->done(p->outfrom, result);
	return result;
}

static int PVDProxyRequest(AObject *object, int reqix, AMessage *msg);
static int PVDProxyOpen(AObject *object, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if ((msg->data == NULL)
	 && (msg->size == 0))
	{
		if (proactive_io == NULL)
			return -EINVAL;

		release_s(p->client, AObjectRelease, NULL);
		int result = AObjectCreate(&p->client, NULL, proactive_io, NULL);
		if (result < 0)
			return result;

		AMsgInit(&p->inmsg, AMsgType_Unknown, NULL, 0);
		AMsgInit(&p->outmsg, AMsgType_Option, proactive_io, 0);
		p->inmsg.done = &PVDProactiveOpenDone;
		p->outmsg.done = &PVDProactiveOpenDone;

		p->outfrom = msg;
		result = p->client->open(p->client, &p->outmsg);
		if (result > 0)
			result = PVDProactiveOpenStatus(p, result);
		return result;
	}

	if (p->client == NULL)
		return -ENOENT;

	return PVDProxyRequest(object, Aio_Input, msg);

	/*if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	release_s(p->client, AObjectRelease, NULL);
	p->client = (AObject*)msg->data;
	AObjectAddRef(p->client);
	return 1;*/
}

static int PVDProxyRequest(AObject *object, int reqix, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if (reqix != Aio_Input) {
		if ((reqix != Aio_Output) || (p->proactive_id == 0))
			return -ENOSYS;

		return p->client->request(p->client, reqix, msg);
	}

	int result = SliceReserve(&p->outbuf, max(msg->size,1024), 2048);
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

static int PVDProxyClose(AObject *object, AMessage *msg)
{
	PVDProxy *p = to_proxy(object);
	if (p->client == NULL)
		return -ENOENT;
	return p->client->close(p->client, msg);
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
	if (int(tick-rt_active) > 10*1000) {
		TRACE("realtime timeout...\n");
		rt->close(rt, NULL);
		rt_active = tick;
	}
	do {
		switch (sm->heart.uCmd)
		{
		case NET_SDVR_LOGIN: result = NET_SDVR_GET_DVRTYPE; break;
		case NET_SDVR_GET_DVRTYPE: result = NET_SDVR_SUPPORT_FUNC; break;
		case NET_SDVR_SUPPORT_FUNC: result = NET_SDVR_DEVICECFG_GET; break;
		case NET_SDVR_DEVICECFG_GET:
			if (login_data.byChanNum != 1)
				result = NET_SDVR_DEVICECFG_GET_EX;
			else
				result = NET_SDVR_NETCFG_GET;
			break;
		case NET_SDVR_DEVICECFG_GET_EX: result = NET_SDVR_NETCFG_GET; break;
		case NET_SDVR_NETCFG_GET: result = NET_SDVR_WORK_STATE; break;
		case NET_SDVR_WORK_STATE:
			if (login_data.byChanNum == 1) {
				sm->msg.type = AMsgType_Private|NET_SDVR_IPCWORKPARAM_GET;
				sm->msg.data = (char*)&sm->heart;
				sm->msg.size = PVDCmdEncode(0, &sm->heart, NET_SDVR_IPCWORKPARAM_GET, sizeof(sm->ipcreq));

				memset(&sm->ipcreq, 0, sizeof(sm->ipcreq));
				sm->ipcreq.cbStreamType = 1;
				result = ioInput(sm->object, &sm->msg);
				continue;
			}
		default:
			if (sm->msg.type != (AMsgType_Private|NET_SDVR_SHAKEHAND)) {
				result = NET_SDVR_SHAKEHAND;
				break;
			}
			sm->msg.type = 0;
			AOperatorTimewait(&sm->timer, NULL, 3*1000);
			return;
		}
		sm->msg.type = AMsgType_Private|result;
		sm->msg.data = (char*)&sm->heart;
		sm->msg.size = PVDCmdEncode(0, &sm->heart, result, 0);
		result = ioInput(sm->object, &sm->msg);
	} while (result > 0);
	if (result < 0) {
		sm->msg.type = AMsgType_Option;
		AOperatorTimewait(&sm->timer, NULL, 3*1000);
	}
}
static int PVDSendDone(AMessage *msg, int result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result < 0) {
		sm->msg.type = AMsgType_Option;
	}
	AOperatorTimewait(&sm->timer, NULL, 3*1000);
	return result;
}
static void PVDDoOpen(AOperator *asop, int result);
static int PVDCloseDone(AMessage *msg, int result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (pvd == NULL) {
		HeartMsgFree(sm, result);
	} else {
		sm->timer.callback = &PVDDoOpen;
		AOperatorTimewait(&sm->timer, NULL, 10*1000);
	}
	return result;
}
static void PVDDoClose(HeartMsg *sm, int result)
{
	TRACE("%s result = %d.\n", (sm->object==pvd)?"client":"realtime", result);

	AMsgInit(&sm->msg, AMsgType_Unknown, NULL, 0);
	sm->msg.done = &PVDCloseDone;
	result = sm->object->close(sm->object, &sm->msg);
	if (result != 0)
		PVDCloseDone(&sm->msg, result);
}
static int PVDRecvDone(AMessage *msg, int result)
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
		case NET_SDVR_IPCWORKPARAM_GET: ptr = &ipc_work; len = sizeof(ipc_work); break;
		case NET_SDVR_WORK_STATE: ptr = &work_status; len = sizeof(work_status); break;
		}
		if (ptr != NULL) {
			memcpy(ptr, phead+1, min(len,msg->size-sizeof(pvdnet_head)));
		}
	}

	AOperatorTimewait(&sm->timer, NULL, 0);
	return result;
}
static void PVDDoRecv(AOperator *asop, int result)
{
	HeartMsg *sm = container_of(asop, HeartMsg, timer);
	if ((result < 0) || (pvd == NULL)) {
		HeartMsgFree(sm, result);
		return;
	}

	if (sm->object == rt) {
		rt_active = GetTickCount();
		AMsgInit(&sm->msg, AMsgType_RefsMsg, &rt_msg, 0);
	} else {
		AMsgInit(&sm->msg, AMsgType_Unknown, NULL, 0);
	}
	result = sm->object->request(sm->object, sm->reqix, &sm->msg);
	if (result != 0) {
		sm->msg.done(&sm->msg, result);
	}
}

static int PVDOpenDone(AMessage *msg, int result)
{
	HeartMsg *sm = container_of(msg, HeartMsg, msg);
	if (result < 0) {
		PVDDoClose(sm, result);
		return result;
	}

	TRACE("%s result = %d.\n", (sm->object==pvd)?"client":"realtime", result);
	if (sm->object == pvd) {
		AOption opt;
		AOptionInit(&opt, NULL);

		strcpy_sz(opt.name, "login_data");
		opt.extend = &login_data;
		sm->object->getopt(sm->object, &opt);

		strcpy_sz(opt.name, "session_id");
		sm->object->getopt(sm->object, &opt);
		userid = atol(opt.value);

		AOption *opt2 = AOptionFind(sm->option, "channel_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byChanNum = atol(opt2->value);

		opt2 = AOptionFind(sm->option, "alarm_in_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byAlarmInPortNum = atol(opt2->value);

		opt2 = AOptionFind(sm->option, "alarm_out_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byAlarmOutPortNum = atol(opt2->value);

		opt2 = AOptionFind(sm->option, "hdd_count");
		if ((opt2 != NULL) && (opt2->value[0] != '\0'))
			login_data.byDiskNum = atol(opt2->value);
	} else {
		rt_active = GetTickCount();
	}

	sm->msg.done = &PVDRecvDone;
	sm->timer.callback = &PVDDoRecv;
	AOperatorTimewait(&sm->timer, NULL, 0);
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
		AOptionInit(&opt, NULL);

		strcpy_sz(opt.name, "version");
		sprintf(opt.value, "%d", login_data.byDVRType);
		sm->object->setopt(sm->object, &opt);

		strcpy_sz(opt.name, "session_id");
		sprintf(opt.value, "%ld", userid);
		sm->object->setopt(sm->object, &opt);
	}

	AMsgInit(&sm->msg, AMsgType_Option, sm->option, 0);
	sm->msg.done = &PVDOpenDone;

	result = sm->object->open(sm->object, &sm->msg);
	if (result != 0)
		PVDOpenDone(&sm->msg, result);
}

int PVDProxyInit(AOption *global_option, AOption *module_option)
{
	if (module_option == NULL)
		return 0;

	proactive_option = AOptionFind(module_option, "proactive");
	if ((proactive_option != NULL) && (atoi(proactive_option->value) == 0))
		proactive_option = NULL;
	if (proactive_option != NULL) {
		proactive_io = AOptionFind(proactive_option, "io");
		proactive_prefix = AOptionChild(proactive_option, "prefix");
		proactive_first = AOptionChildInt(proactive_option, "first", 1);
	}

	force_alarm = AOptionChildInt(module_option, "force_alarm", FALSE);

	int result = -EFAULT;
	AOption opt;
	AOptionInit(&opt, NULL);

	AModule *syncControl = AModuleFind("stream", "SyncControl");
	if (syncControl != NULL) {
		strcpy_sz(opt.name, "stream");
		strcpy_sz(opt.value, "PVDClient");
		result = AObjectCreate2(&pvd, NULL, &opt, syncControl);
	}
	HeartMsg *sm = NULL;
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; AObjectAddRef(pvd);
		sm->option = NULL;

		sm->msg.type = AMsgType_Option;
		sm->msg.done = &PVDSendDone;
		sm->heart.uCmd = NET_SDVR_LOGIN;
		sm->timer.callback = &PVDDoSend;
		AOperatorTimewait(&sm->timer, NULL, 3*1000);
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = pvd; AObjectAddRef(pvd);
		sm->option = AOptionClone(module_option, NULL);
		sm->reqix = Aio_Output;
		sm->threadix = 1;
		sm->timer.callback = &PVDDoOpen;
		AOperatorTimewait(&sm->timer, NULL, 0);

		strcpy_sz(opt.value, "PVDRTStream");
		result = AObjectCreate2(&rt, NULL, &opt, syncControl);
	}
	if (result >= 0) {
		sm = (HeartMsg*)malloc(sizeof(HeartMsg));
		if (sm == NULL)
			result = -ENOMEM;
	}
	if (result >= 0) {
		sm->object = rt; AObjectAddRef(rt);
		sm->option = AOptionClone(module_option, NULL);
		sm->reqix = 0;
		sm->threadix = 2;
		sm->timer.callback = &PVDDoOpen;
		AOperatorTimewait(&sm->timer, NULL, 3*1000);
	}
	return result;
}

static void PVDProxyExit(void)
{
	if (pvd != NULL) {
		//pvd->cancel(pvd, ARequest_MsgLoop|Aio_Output, NULL);
		pvd->close(pvd, NULL);
		AObjectRelease(pvd);
		pvd = NULL;
	}
	if (rt != NULL) {
		//rt->cancel(rt, ARequest_MsgLoop|0, NULL);
		rt->close(rt, NULL);
		AObjectRelease(rt);
		rt = NULL;
	}
	release_s(rt_msg.buf, ARefsBufRelease, NULL);
}

static int PVDProxyProbe(AObject *object, AMessage *msg)
{
	int result = PVDCmdDecode(0, msg->data, msg->size);
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
	&PVDProxyClose,
};
