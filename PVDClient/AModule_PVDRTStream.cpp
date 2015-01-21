#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "PvdNetCmd.h"


struct PVDRTStream {
	AObject   object;
	AObject  *io;
	PVDStatus status;
	DWORD     userid;
	DWORD     version;

	AMessage    outmsg;
	AMessage   *outfrom;
	SliceBuffer outbuf;
};
#define to_rt(obj) container_of(obj, PVDRTStream, object)
#define from_outmsg(msg) container_of(msg, PVDRTStream, outmsg)

static void PVDRTRelease(AObject *object)
{
	PVDRTStream *rt = to_rt(object);
	release_s(rt->io, AObjectRelease, NULL);
	SliceFree(&rt->outbuf);

	free(rt);
}

static long PVDRTCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDRTStream *rt = (PVDRTStream*)malloc(sizeof(PVDRTStream));
	if (rt == NULL)
		return -ENOMEM;

	extern AModule PVDRTModule;
	AObjectInit(&rt->object, &PVDRTModule);

	rt->io = NULL;
	rt->status = pvdnet_invalid;
	rt->userid = 0;
	rt->version = PROTOCOL_V3;

	rt->outfrom = NULL;
	SliceInit(&rt->outbuf);

	if (parent != NULL) {
		AOption opt;
		memset(&opt, 0, sizeof(opt));
		INIT_LIST_HEAD(&opt.children_list);
		INIT_LIST_HEAD(&opt.brother_entry);

		strcpy_s(opt.name, "session_id");
		if (parent->getopt(parent, &opt) >= 0)
			rt->userid = atol(opt.value);

		strcpy_s(opt.name, "version");
		if (parent->getopt(parent, &opt) >= 0)
			rt->version = atol(opt.value);
	}

	AOption *io_option = AOptionFindChild(option, "io");
	long result = AObjectCreate(&rt->io, &rt->object, io_option, "tcp");

	*object = &rt->object;
	return result;
}

static long PVDRTOpenStatus(PVDRTStream *rt, long result)
{
	pvdnet_head *phead;
	do {
		if (result < 0)
			rt->status = pvdnet_closing;
		switch (rt->status)
		{
		case pvdnet_connecting:
			SliceReset(&rt->outbuf);
			if (SliceResize(&rt->outbuf, 64*1024) < 0) {
				result = -ENOMEM;
				break;
			}
		{
			AOption *option = (AOption*)rt->outfrom->data;
			if (rt->version != PROTOCOL_V3) {
				result = sizeof(STRUCT_SDVR_REALPLAY);
			} else {
				result = sizeof(STRUCT_SDVR_REALPLAY_EX);
			}

			rt->outmsg.type = AMsgType_Custom|NET_SDVR_REAL_PLAY;
			rt->outmsg.data = SliceCurPtr(&rt->outbuf);
			rt->outmsg.size = PVDCmdEncode(rt->userid, rt->outmsg.data, NET_SDVR_REAL_PLAY, result);

			memset(rt->outmsg.data+sizeof(pvdnet_head), 0, result);
			STRUCT_SDVR_REALPLAY *rp = (STRUCT_SDVR_REALPLAY*)(rt->outmsg.data+sizeof(pvdnet_head));

			AOption *channel = AOptionFindChild(option, "channel");
			if (channel != NULL)
				rp->byChannel = atol(channel->value);

			AOption *linkmode = AOptionFindChild(option, "linkmode");
			if (linkmode != NULL)
				rp->byLinkMode = atol(linkmode->value);
		}
			rt->status = pvdnet_syn_login;
			result = rt->io->request(rt->io, ARequest_Input, &rt->outmsg);
			break;

		case pvdnet_syn_login:
			rt->status = pvdnet_ack_login;
			AMsgInit(&rt->outmsg, AMsgType_Unknown, SliceResPtr(&rt->outbuf), SliceResLen(&rt->outbuf));
			result = rt->io->request(rt->io, ARequest_Output, &rt->outmsg);
			break;

		case pvdnet_ack_login:
			SlicePush(&rt->outbuf, result);
			if (SliceCurLen(&rt->outbuf) < sizeof(pvdnet_head)) {
				if (rt->outmsg.type & AMsgType_Custom)
					SliceReset(&rt->outbuf);
				rt->status = pvdnet_syn_login;
				break;
			}
			phead = (pvdnet_head*)SliceCurPtr(&rt->outbuf);
			if (phead->uFlag == NET_CMD_HEAD_FLAG) {
				result = PVDCmdDecode(rt->userid, SliceCurPtr(&rt->outbuf), SliceCurLen(&rt->outbuf));
				if (result < 0)
					break;
				if (result > SliceCurLen(&rt->outbuf)) {
					if (rt->outmsg.type & AMsgType_Custom) {
						result = -EFAULT;
					} else {
						rt->status = pvdnet_syn_login;
					}
					break;
				}
				SlicePop(&rt->outbuf, result);
				if (!phead->uResult) {
					result = -EFAULT;
					break;
				}
				if (SliceCurLen(&rt->outbuf) < sizeof(pvdnet_head)) {
					rt->status = pvdnet_syn_login;
					break;
				}
				phead = (pvdnet_head*)SliceCurPtr(&rt->outbuf);
			}
			if (phead->uFlag == MSHEAD_FLAG) {
				rt->status = pvdnet_con_devinfo;
				return result;
			}
			if (phead->uFlag == MSHDV2_FLAG) {
				rt->status = pvdnet_con_devinfo2;
				return result;
			}
			if (phead->uFlag == STREAM_HEADER_FLAG) {
				rt->status = pvdnet_con_devinfox;
				return result;
			}
			result = -ENOSYS;
			break;

		case pvdnet_closing:
			AMsgInit(&rt->outmsg, AMsgType_Unknown, NULL, 0);
			rt->status = pvdnet_disconnected;
			result = rt->io->close(rt->io, &rt->outmsg);
			if (result == 0)
				return 0;

		case pvdnet_disconnected:
			rt->status = pvdnet_invalid;
			return -EFAULT;

		default:
			assert(FALSE);
			return -EACCES;
		}
	} while (result != 0);
	return result;
}

static long PVDRTOpenDone(AMessage *msg, long result)
{
	PVDRTStream *rt = from_outmsg(msg);
	msg = rt->outfrom;

	result = PVDRTOpenStatus(rt, result);
	if (result != 0)
		result = msg->done(msg, result);
	return result;
}

static long PVDRTOpen(AObject *object, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	rt->outmsg.type = AMsgType_Option;
	rt->outmsg.data = (char*)AOptionFindChild((AOption*)msg->data, "io");
	rt->outmsg.size = sizeof(AOption);

	rt->outmsg.done = &PVDRTOpenDone;
	rt->outfrom = msg;

	rt->status = pvdnet_connecting;
	long result = rt->io->open(rt->io, &rt->outmsg);
	if (result != 0)
		result = PVDRTOpenStatus(rt, result);
	return result;
}

static long PVDRTOutputStatus(PVDRTStream *rt, long result)
{
	do {
		SlicePush(&rt->outbuf, result);
		pvdnet_head *phead = (pvdnet_head*)SliceCurPtr(&rt->outbuf);
		result = SliceCurLen(&rt->outbuf);

		if (result < 4) {
			result = 0;
		} else if (ISMSHEAD(phead)) {
			assert((rt->status == pvdnet_con_devinfo) || (rt->status == pvdnet_con_devinfo2));
			if (result < sizeof(MSHEAD)) {
				result = 0;
			} else {
				result = MSHEAD_GETFRAMESIZE(phead);
			}
		} else if (phead->uFlag == STREAM_HEADER_FLAG) {
			assert(rt->status == pvdnet_con_devinfox);
			if (result < sizeof(STREAM_HEADER)) {
				result = 0;
			} else {
				STREAM_HEADER *p = (STREAM_HEADER*)phead;
				result = p->nHeaderSize + p->nEncodeDataSize;
			}
		} else if (phead->uFlag == NET_CMD_HEAD_FLAG) {
			if (result < sizeof(pvdnet_head)) {
				result = 0;
			} else {
				result = PVDCmdDecode(rt->userid, phead, result);
				if (result < 0) {
					TRACE("session(%d): id(%d) error.\n", rt->userid, phead->uUserId);
					break;
				}
			}
		} else {
			TRACE("session(%d): unsupport format: 0x%p.\n", rt->userid, phead->uFlag);
			result = -ENOSYS;
			break;
		}
		if ((result == 0) || (result > SliceCurLen(&rt->outbuf))) {
			if (max(result,20) > SliceCapacity(&rt->outbuf)) {
				result = SliceResize(&rt->outbuf, result);
				if (result < 0)
					break;
			}
			if (rt->outmsg.type & AMsgType_Custom) {
				SliceReset(&rt->outbuf);
			}
			AMsgInit(&rt->outmsg, AMsgType_Unknown, SliceResPtr(&rt->outbuf), SliceResLen(&rt->outbuf));
			result = rt->io->request(rt->io, ARequest_Output, &rt->outmsg);
			continue;
		}
		SlicePop(&rt->outbuf, result);

		AMessage *msg = rt->outfrom;
		msg->type = AMsgType_Custom|rt->status;
		if ((msg->data != NULL) && (msg->size != 0)) {
			memcpy(msg->data, phead, min(msg->size,result));
		} else {
			msg->data = (char*)phead;
			msg->size = result;
		}
		break;
	} while (result > 0);
	return result;
}

static long PVDRTOutputDone(AMessage *msg, long result)
{
	PVDRTStream *rt = from_outmsg(msg);
	if (result >= 0)
		result = PVDRTOutputStatus(rt, result);
	if (result != 0)
		result = rt->outfrom->done(rt->outfrom, result);
	return result;
}

static long PVDRTRequest(AObject *object, long reqix, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	if (reqix != 0)
		return msg->size;

	rt->outmsg.done = &PVDRTOutputDone;
	rt->outfrom = msg;
	return PVDRTOutputStatus(rt, 0);
}

static long PVDRTClose(AObject *object, AMessage *msg)
{
	PVDRTStream *rt = to_rt(object);
	return rt->io->close(rt->io, NULL);
}

AModule PVDRTModule = {
	"stream",
	"PVDRTStream",
	sizeof(PVDRTStream),
	NULL, NULL,
	&PVDRTCreate,
	&PVDRTRelease,
	1,

	&PVDRTOpen,
	NULL,
	NULL,
	&PVDRTRequest,
	NULL,
	&PVDRTClose,
};
